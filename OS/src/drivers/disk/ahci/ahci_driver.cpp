#include "drivers/disk/ahci/ahci_cmd.h"
#include "drivers/disk/ahci/hba_port.h"
#include "drivers/disk/disk_driver.h"
#include "drivers/fs/gpt.h"
#include <types.h>
#include <drivers/disk/ahci/ahci.h>
#include <stdlib/stdio.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/mem.h>
#include <kernel/vfs/file.h>
#include <stdlib/string.h>


namespace DISK {

extern hba_mem_t *hba_memory;
extern uint8_t device_count;
extern AHCIDevice* devices[32];

// class AHCIDevice

AHCIDevice::AHCIDevice(uint8_t port_num)
{
    this->port_num = port_num;
    this->port = &hba_memory->ports[port_num];

    //Add the block device

    this->interface.disk_number = port_num;
    this->interface.driver = this;
    this->interface.type = diskTypes::AHCI;
    /*
    char *new_filename = (char*)kcalloc(1, 4);
    std::strcpy(new_filename, "hd");
    std::strcat(new_filename, std::itoa(hd_cnt++, 10));
    VFS::file_t* block = VFS::get_root()->get_subdir("dev")->create_child(new_filename, VFS::DEVICE);
    block->file_data = (void*)this->get_interface();
    block->permissions = 0xFF;
    */
    DISK::disks[DISK::disk_count++] = this->get_interface();

    //Stop the port from doing any commands

    port->command_status.cmd_start = 0;
    port->command_status.FIS_recieve_enable = 0;

    //Wait for port to stop

    while(true)
    {
        if(port->command_status.FIS_recieve_running)
            continue;
        
        if(port->command_status.cmd_list_running)
            continue;
        
        break;
    }

    //Allocate memory for data structuress

    port->cmd_list_base_addr = (command_hdr_t*)kernel::allocate_pages(1);
    port->fis_base_addr = (HBA_FIS*)kernel::allocate_pages(1);

    //Allocate memory for command table slots
    
    for(int i = 0; i < 32; i++)
    {
        port->cmd_list_base_addr[i].cmd_table_desc_base_addr = (command_tbl_t*)kernel::allocate_pages(1);
    }
    
    //Enable commands in the port again

    while(port->command_status.cmd_list_running);
    port->command_status.FIS_recieve_enable = 1;
    port->command_status.cmd_start = 1;

    //Read the GPT data
    this->read_gpt();
}

void AHCIDevice::read_gpt()
{
    gpt_data = (FS::gpt_part_data*)kernel::allocate_pages(4);
    kernel::map_pages((uint64_t)gpt_data, (uint64_t)gpt_data, 4);
    FS::gpt_part_table_hdr *gpt_hdr = &gpt_data->hdr;

    //Read the GPT header, located at LBA 1
    this->read(1, 1, (uint8_t*)gpt_hdr);

    //Read the partition table until ESP found
    int currentEntry = 0;
    uint32_t lba;

    int gpt_entry_sectors = (gpt_hdr->part_entry_count * gpt_hdr->part_entry_size) / 0x200;

    for(lba = gpt_hdr->lba_part_entry_arr; lba < gpt_entry_sectors + gpt_hdr->lba_part_entry_arr; lba++)
    {
        this->read(lba, 1, (uint8_t*)&gpt_data->entries[currentEntry]);
        for(int i = 0; i < 4; i++) {
            uint64_t part_size = (gpt_data->entries[currentEntry].ending_lba - gpt_data->entries[currentEntry].starting_lba ) * 512;
            Log.v(
                "AHCI Disks",
                "(hd%u, gpt%u) \tlabel: %l\tsize: %x", port_num, currentEntry, gpt_data->entries[currentEntry].parition_name, part_size);
            currentEntry++;
            if(gpt_data->entries[currentEntry].starting_lba == 0) return;
        }
    }
}

FS::gpt_part_data* AHCIDevice::get_gpt()
{
    return gpt_data;
}

#define AHCI_ERR_DEV_BUSY           -2
#define AHCI_ERR_CONTROLLER_ERR     -3
#define AHCI_SUCCESS                 0

int AHCIDevice::read(uint64_t starting_lba, uint32_t sector_cnt, uint8_t *dma_buffer)
{
    //Clear pending interrupts

    *((uint32_t*)(&port->interupt_status)) = -1;

    int slot = getSlot();
    if(slot == -1)
    {
        return AHCI_ERR_DEV_BUSY;
    }

    command_hdr_t *command_header = &port->cmd_list_base_addr[slot];
    command_header->cmdfis_length = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    command_header->write_command = 0;
    command_header->prdt_length = (uint16_t)((sector_cnt - 1) >> 4) + 1;

    //Set up the PRDT

    command_tbl_t *command_table = command_header->cmd_table_desc_base_addr;
    memset_8(command_table, sizeof(command_tbl_t), 0);

    int i;
    for(i = 0; i < command_header->prdt_length - 1; i++)
    {
        command_table->prdt[i].data_base_address = (uint64_t)dma_buffer;
        command_table->prdt[i].data_byte_count = 8 * 1024 - 1;
        command_table->prdt[i].intr_on_completion = 1;
        dma_buffer += 0x2000;
        sector_cnt -= 16;
    }


    command_table->prdt[i].data_base_address = (uint64_t)dma_buffer;
    command_table->prdt[i].data_byte_count = (sector_cnt << 9) - 1;
    command_table->prdt[i].intr_on_completion = 1;

    //Set up the command

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&command_table->cmd_fis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0x25;

    uint32_t startl = starting_lba & 0xFFFF;
    uint32_t starth = starting_lba >> 32;

    cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);

	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);

    cmdfis->countl = sector_cnt & 0xFF;
    cmdfis->counth = sector_cnt >> 8;

    
    return run_command(slot);
}

int AHCIDevice::write(uint64_t starting_lba, uint32_t sector_cnt, uint8_t *data_buffer)
{
    //Clear pending interrupts

    *((uint32_t*)(&port->interupt_status)) = -1;

    int slot = getSlot();
    if(slot == -1)
    {
        return AHCI_ERR_DEV_BUSY;
    }

    command_hdr_t *command_header = &port->cmd_list_base_addr[slot];
    command_header->cmdfis_length = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    command_header->write_command = 1;
    command_header->prdt_length = (uint16_t)((sector_cnt - 1) >> 4) + 1;

    //Set up the PRDT

    command_tbl_t *command_table = command_header->cmd_table_desc_base_addr;
    memset_8(command_table, sizeof(command_tbl_t), 0);

    int i;
    for(i = 0; i < command_header->prdt_length - 1; i++)
    {
        command_table->prdt[i].data_base_address = (uint64_t)data_buffer;
        command_table->prdt[i].data_byte_count = 8 * 1024 - 1;
        command_table->prdt[i].intr_on_completion = 1;
        data_buffer += 0x2000;
        sector_cnt -= 16;
    }


    command_table->prdt[i].data_base_address = (uint64_t)data_buffer;
    command_table->prdt[i].data_byte_count = (sector_cnt << 9) - 1;
    command_table->prdt[i].intr_on_completion = 1;

    //Set up the command

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&command_table->cmd_fis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0xCA;

    uint32_t startl = starting_lba & 0xFFFF;
    uint32_t starth = starting_lba >> 32;

    cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);

	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);

    cmdfis->countl = sector_cnt & 0xFF;
    cmdfis->counth = sector_cnt >> 8;

    return run_command(slot);
}

#define AHCI_DEVICE_IDENTIFY_CMD 0xEC

int AHCIDevice::identifyDevice(uint16_t *_buffer)
{
    int slot = getSlot();
    if(slot == -1)
    {
        return AHCI_ERR_DEV_BUSY;
    }

    command_hdr_t *command_header = &port->cmd_list_base_addr[slot];
    command_header->cmdfis_length = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    command_header->prdt_length = 1;
    command_header->prdt_byte_count = 512;

    //Set up the PRDT

    command_tbl_t *command_table = command_header->cmd_table_desc_base_addr;
    memset_8(command_table, sizeof(command_tbl_t), 0);
    
    command_table->prdt->data_base_address = (uint64_t)_buffer;
    command_table->prdt->data_byte_count = 512;
    command_table->prdt->intr_on_completion = 1;

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&command_table->cmd_fis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = AHCI_DEVICE_IDENTIFY_CMD;
    cmdfis->device = 1 << 6;

    FIS_PIO_SETUP *piofis = &port->fis_base_addr->pio_setup_fis;
    piofis->fis_type = FIS_TYPE_PIO_SETUP;
    piofis->transfer_cnt = 0;
    piofis->device = 1 << 6;
    piofis->i = 1;
    piofis->d = 1;

    int status = run_command(slot);
    if(status != AHCI_SUCCESS)
    {
        return status;
    }

    return AHCI_SUCCESS;
}

int AHCIDevice::getSlot()
{
    //Return the first avalible command slot
    uint32_t avalible_slots = port->sata_active | port->command_issue;
    for(int current_slot = 0; current_slot < 32; current_slot++)
    {
        if(!((1 << current_slot) & avalible_slots))
        {
            return current_slot;
        }
    }

    //No slots found!
    return -1;
}

int AHCIDevice::run_command(int slot)
{
    //Wait for port to be ready

    int spin = 0;
    while(port->task_file_data.status_busy && port->task_file_data.status_data_transfer_requested)
    {
        if(spin >= 100000)
        {
            return AHCI_ERR_DEV_BUSY;
        }

        spin++;
    }

    port->command_issue = 1 << slot;

    //Wait for port to finish

    while(port->command_issue & (1 << slot))
    {
        if(port->interupt_status.task_file_error_sts)
        {
            return AHCI_ERR_CONTROLLER_ERR;
        }
    }
    return AHCI_SUCCESS;
}

rw_disk_t *AHCIDevice::get_interface()
{
    return &this->interface;
}

}


