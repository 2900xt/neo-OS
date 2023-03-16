#include "drivers/ahci/ahci_cmd.h"
#include "drivers/ahci/hba_fis.h"
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

    char* new_filename = (char*)kcalloc(1, 5);
    if(port->signature == SIGNATURE_ATA)
    {
        std::strcpy(new_filename, "hd");
        std::strcat(new_filename, std::itoa(hd_cnt++, 10));
    }
    else {
        std::strcpy(new_filename, "cdrom");
        std::strcat(new_filename, std::itoa(cd_cnt++, 10));
    }

    VFS::file_t* block = VFS::get_root()->get_subdir("dev")->create_child(new_filename, VFS::DEVICE);
    block->file_data = (void*)this;
    block->permissions = 0xFF;

    stop_command();

    //Get 3 * 4K pages for data
    //Then map them into virtual memory

    uint64_t page = (uint64_t)kernel::allocate_pages(3);
    kernel::map_pages(page, page + 0x3000, page, page + 0x3000);

    //Allocate a 1K memory space for the command list

    port->cmd_list_base_addr_high = page >> 32;
    port->cmd_list_base_addr_low = page & 0xFFFF;
    this->cmd_list = (HBA_CMD_HEADER*)page;
    memset_8(cmd_list, 0x400, 0);

    page += 0x400;

    //Allocate a 256 byte memory space for the FIS

    port->fis_base_addr_high = page >> 32;
    port->fis_base_addr_low = page & 0xFFFF;
    this->port_fis = (HBA_FIS*)page;

    memset_8((void*)port_fis, 0x100, 0);

    page += 0x100;

    //Allocate a 8K memory region for the command table
    //Each command table should take up 256 bytes

    for(int i = 0; i < 32; i++)
    {
        cmd_list[i].prdtl = 8;  //* PRDT entries per command table
        cmd_list[i].ctba = page & 0xFFFF;
        cmd_list[i].ctbau = page >> 32;
        
        memset_8((void*)page, 0x100, 0);
        page += 0x100;
    }

    start_command();
}

void AHCIDevice::start_command()
{
    //Wait till command is done
    while(port->command_status.cmd_list_running);

    //Start command
    port->command_status.FIS_recieve_enable = 1;
    port->command_status.cmd_start = 1;
}

void AHCIDevice::stop_command()
{
    port->command_status.cmd_start = 0;
    port->command_status.FIS_recieve_enable = 0;

    while(true)
    {
        if(port->command_status.FIS_recieve_running) continue;
        if(port->command_status.cmd_list_running) continue;
        break;
    }
}

int AHCIDevice::find_cmd_slot()
{
    uint32_t slots = port->sata_active | port->command_issue;
    for(int i = 0; i < 32; i++)
    {
        if(!(slots & (1 << i))) return i;
    }
    std::klogf("AHCI: Could not find a command entry\n");
    return -1;
}

bool AHCIDevice::read_sectors(uint64_t lba, uint32_t count, void *_buffer)
{
    
    uint64_t buffer = (uint64_t)_buffer;
    uint32_t startl = lba & 0xFFFF;
    uint32_t starth = lba >> 32;

    *(uint32_t*)(&port->interupt_status) = (uint32_t)-1;    //Clear pending interrupts
    int slot = find_cmd_slot();
    if(slot == -1) return false;

    HBA_CMD_HEADER *cmd_header = cmd_list;
    cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmd_header->w = 0;                                          //Read / Write
    cmd_header->prdtl = 1;

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)((uint64_t)cmd_header->ctba);
    memset_8(cmdtbl, sizeof(HBA_CMD_TBL), 0);

    cmdtbl->prdt_entry[0].dba = buffer & 0xFFFF;
    cmdtbl->prdt_entry[0].dbau = buffer >> 32;
    cmdtbl->prdt_entry[0].dbc = (count << 9) - 1;
    cmdtbl->prdt_entry[0].i = 1;

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0x25;

    cmdfis->lba0 = (uint8_t)(startl);
    cmdfis->lba1 = (uint8_t)(startl >> 8);
    cmdfis->lba2 = (uint8_t)(startl >> 16);
    cmdfis->lba3 = (uint8_t)(startl >> 24);
    cmdfis->lba4 = (uint8_t)(starth);
    cmdfis->lba5 = (uint8_t)(starth >> 8);
    
    cmdfis->device = 1 << 6; //LBA mode

    cmdfis->countl = count & 0xFF;
    cmdfis->counth = (count >> 8) & 0xFF;

    int spin = 0;

    while((port->task_file_data.status_busy || port->task_file_data.status_data_transfer_requested) && (spin < 1000000))
    {
        spin++;
    }

    if(spin >=1000000)
    {
        std::klogf("Device is busy!\n");
        return false;
    }

    port->command_issue = 1 ;

    while(true)
    {
        if((port->command_issue & (1 )) == 0) break;
        if(port->interupt_status.task_file_error_sts)
        {
            std::klogf("Disk read error - Task file interrupt!\n");
            return false;
        }
    }

    if(port->interupt_status.task_file_error_sts)
    {
        std::klogf("Disk read error - Task file interrupt!\n");
        return false;
    }

    return true;
}

}

