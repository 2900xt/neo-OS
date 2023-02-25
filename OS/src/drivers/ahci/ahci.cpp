#include "stdlib/mem.h"
#include "stdlib/stdio.h"
#include "stdlib/stdlib.h"
#include "types.h"
#include "x64/paging.h"
#include <drivers/ahci/ahci.h>
#include <drivers/pci/pci.h>

namespace AHCI {

static hba_mem_t *hba_memory;
static uint64_t port_count;
static port_t ports[32];

#define CMD_RUNNING 0x8000
#define FIS_RUNNING 0x4000
#define FIS_RECIEVE_ENABLE 0x10
#define COMMAND_START 0x1

static void start_cmd(hba_port_t *port)
{
    while(port->command_status & CMD_RUNNING);
    port->command_status |= FIS_RECIEVE_ENABLE;
    port->command_status |= COMMAND_START;
}

static void stop_cmd(hba_port_t *port)
{

    port->command_status &= ~COMMAND_START;
    port->command_status &= ~FIS_RECIEVE_ENABLE;

    while(true)
    {
        if(port->command_status & FIS_RUNNING) continue;
        if(port->command_status & CMD_RUNNING) continue;
        break;
    }
}

static void confiure_port(port_t* port)
{
    stop_cmd(port->port);

    void* command_list_base = AMD64::next_page();
    port->port->cmd_list_base = (uint64_t)command_list_base;
    memset_8((void*)command_list_base, 1024, 0);

    void* fis_base = AMD64::next_page();
    port->port->fis_base_addr = (uint64_t)fis_base;
    memset_8(fis_base, 256, 0);

    hba_command_hdr_t* cmd_header = (hba_command_hdr_t*)command_list_base;

    for(int i = 0; i < 32; i++)
    {
        cmd_header[i].prdt_length = 8;
        void* cmd_table_addr = AMD64::next_page();
        uint64_t address = (uint64_t)cmd_table_addr + (i << 8);
        cmd_header[i].cmd_table_base_addr = address;
        memset_8((void*)address, 256, 0);
    }

    start_cmd(port->port);
}

static void probe_ports()
{
    assert(hba_memory != NULL);
    port_count = 0;

    for(int i = 0; i < 32; i++)
    {
        if(hba_memory->ghc.ports_implemented & (1 << i))
        {
            if(hba_memory->ports[i].sata_status.device_detection != 0x3 || hba_memory->ports[i].sata_status.interface_pm != 0x1) continue;
            if(hba_memory->ports[i].signature != SIGNATURE_ATA && hba_memory->ports[i].signature != SIGNATURE_ATAPI) continue;
            ports[port_count].port_number = i;
            ports[port_count].port = &hba_memory->ports[i];
            ports[port_count].port_type = hba_memory->ports[i].signature;
            confiure_port(&ports[port_count]);
            port_count++;
        }
    }
    port_count--;
}

#define FIS_REG_H2D 0x27
#define CMD_READ_DMA 0x25

int read_drive(port_t* _port, uint64_t start_lba, uint16_t sector_count, void* buffer)
{

    uint32_t sectors_low = (uint32_t) start_lba;
    uint32_t sectors_high = (uint32_t) (start_lba >> 32);

    _port->port->interupt_status = (uint32_t)-1;    //Clear pending interrupts

    hba_command_hdr_t* cmd_hdr = (hba_command_hdr_t*)_port->port->cmd_list_base;
    cmd_hdr->command_fis_length = sizeof(fis_reg_h2d_t);
    cmd_hdr->write = false;                         //disk read
    cmd_hdr->prdt_length = 1;
    
    hba_cmd_table_t* cmd_table = (hba_cmd_table_t*)cmd_hdr->cmd_table_base_addr;
    memset_8(cmd_table, 0, sizeof(hba_cmd_table_t));

    cmd_table->prdtEntries[0].data_base_addr = (uint64_t)buffer;
    cmd_table->prdtEntries[0].byte_count = (sector_count << 9) - 1;
    cmd_table->prdtEntries[0].interrupt_on_completion = 1;

    fis_reg_h2d_t* cmd_fis = (fis_reg_h2d_t*)(&cmd_table->command_fis);
    cmd_fis->fis_type = FIS_REG_H2D;
    cmd_fis->command_ctrl = 1;
    cmd_fis->command = CMD_READ_DMA;
    cmd_fis->lba0 = (uint8_t)sectors_low;
    cmd_fis->lba1 = (uint8_t)(sectors_low >> 8);
    cmd_fis->lba2 = (uint8_t)(sectors_low >> 16);
    cmd_fis->lba3 = (uint8_t)(sectors_high);
    cmd_fis->lba4 = (uint8_t)(sectors_high >> 8);
    cmd_fis->lba5 = (uint8_t)(sectors_high >> 16);

    cmd_fis->device_register = (1 << 6);
    cmd_fis->count = sector_count;

    _port->port->command_issue = 1;

    while(true)
    {
        if(_port->port->command_issue == 0) break;
        if(_port->port->interupt_status & (1 << 30)) return -1; //read error
    }

    return 0;   //read success
}

void ahci_init()
{
    pci_device_t *sata_controller = get_pci_dev(0x1, 0x6, 0x1);
    hba_memory = (hba_mem_t*)((uint64_t)sata_controller->BARS[5]);

    if(!hba_memory->ghc.global_host_ctrl.ahci_enable) return;

    probe_ports();

    klogf(LOG_IMPORTANT, "AHCI Driver Initialized! \n");
}

}