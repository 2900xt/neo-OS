#include "stdlib/stdio.h"
#include "stdlib/stdlib.h"
#include "types.h"
#include <drivers/ahci/ahci.h>
#include <drivers/pci/pci.h>

namespace AHCI {

static hba_mem_t *hba_memory;
static uint64_t port_count;
static port_t ports[32];

static void probe_ports()
{
    assert(hba_memory != NULL);
    port_count = 0;

    for(int i = 0; i < 32; i++)
    {
        if(hba_memory->ghc.ports_implemented & (1 << i))
        {
            if(hba_memory->ports[i].sata_status.device_detection != 0x3 || hba_memory->ports[i].sata_status.interface_pm != 0x1) continue;
            ports[port_count].port_number = i;
            ports[port_count].dma_buffer = NULL;
            ports[port_count].port = &hba_memory->ports[i];
            ports[port_count].port_type = hba_memory->ports[i].signature;
            port_count++;
        }
    }
    port_count--;
}

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

void ahci_init()
{
    pci_device_t *sata_controller = get_pci_dev(0x1, 0x6, 0x1);
    hba_memory = (hba_mem_t*)((uint64_t)sata_controller->BARS[5]);
    probe_ports();
}

}