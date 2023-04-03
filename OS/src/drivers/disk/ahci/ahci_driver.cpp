#include "drivers/ahci/ahci_cmd.h"
#include "drivers/ahci/hba_port.h"
#include <types.h>
#include <drivers/ahci/ahci.h>
#include <stdlib/stdio.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/mem.h>
#include <kernel/vfs/file.h>
#include <stdlib/string.h>


namespace AHCI {

extern hba_mem_t *hba_memory;
extern uint8_t device_count;
extern AHCIDevice* devices[32];

static int hd_cnt = 0, cd_cnt = 0;

// class AHCIDevice

AHCIDevice::AHCIDevice(uint8_t port_num)
{
    this->port_num = port_num;
    this->port = &hba_memory->ports[port_num];

    //Add the block device

    char* new_filename;
    if(port->signature == SIGNATURE_ATA)
    {
        new_filename = (char*)kcalloc(1, 4);
        std::strcpy(new_filename, "hd");
        std::strcat(new_filename, std::itoa(hd_cnt++, 10));
    }
    else {
        new_filename = (char*)kcalloc(1, 7);
        std::strcpy(new_filename, "cdrom");
        std::strcat(new_filename, std::itoa(cd_cnt++, 10));
    }

    VFS::file_t* block = VFS::get_root()->get_subdir("dev")->create_child(new_filename, VFS::DEVICE);
    block->file_data = (void*)this;
    block->permissions = 0xFF;

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
    port->fis_base_addr = (uint64_t)kernel::allocate_pages(1);

    //Allocate memory for one command table slot
    
    port->cmd_list_base_addr->cmd_table_desc_base_addr = (command_tbl_t*)kernel::allocate_pages(1);
    
    //Enable commands in the port again

    while(port->command_status.cmd_list_running);
    port->command_status.FIS_recieve_enable = 1;
    port->command_status.cmd_start = 1;
}

#define AHCI_ERR_TOO_MANY_SECTORS -1
#define AHCI_READ_SUCCESS 0

int AHCIDevice::read(uint64_t starting_lba, uint32_t sector_cnt, void *dma_buffer)
{
    //Clear pending interrupts

    *((uint32_t*)(&port->interupt_status)) = -1;

    command_hdr_t *command_header = port->cmd_list_base_addr;
    command_header->cmdfis_length = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    command_header->write_command = 0;
    command_header->prdt_length = (uint16_t)((sector_cnt - 1) >> 4) + 1;
    if(command_header->prdt_length >= 9) return AHCI_ERR_TOO_MANY_SECTORS;

    //Set up the PRDT

    command_tbl_t *command_table = command_header->cmd_table_desc_base_addr;
    memset_8(command_table, sizeof(command_tbl_t), 0);

    int i;
    for(i = 0; i < command_header->prdt_length - 1; i++)
    {
        command_table->prdt[i].data_base_address = (uint64_t)dma_buffer;
        command_table->prdt[i].data_byte_count = 8 * 1024 - 1;
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

    //Wait for port to be ready

    while(port->task_file_data.status_busy && port->task_file_data.status_data_transfer_requested);

    port->command_issue = 1;

    //Wait for port to finish

    while(port->command_issue & 1);
 
    std::klogf("0x%x\n", cmdfis);

    return AHCI_READ_SUCCESS;
}

}


