#include "types.h"
#include <stdlib/stdlib.h>
#include <kernel/mem/paging.h>
#include <drivers/ahci/ahci.h>
#include <drivers/pci/pci.h>

namespace AHCI {

static hba_mem_t *hba_memory;
static int8_t port_count;
static port_t ports[32];

static void probe_ports()
{
    assert(hba_memory != NULL);
    port_count = -1;

    for(int i = 0; i < 32; i++)
    {
        if(hba_memory->ghc.ports_implemented & (1 << i))
        {
            bool ok = false;
            switch(hba_memory->ports[i].signature)
            {
                case SIGNATURE_ATA:
                case SIGNATURE_ATAPI:
                    ok = true;
                    break;
            }

            if(!ok) continue;

            if(hba_memory->ports[i].sata_status.device_detection != 0x3 || hba_memory->ports[i].sata_status.interface_pm != 0x1) continue;

            port_count++;
            ports[port_count].port_number = i;
            ports[port_count].port = &hba_memory->ports[i];
            ports[port_count].port_type = hba_memory->ports[i].signature;
        }
    }
}

void ahci_init()
{
    PCI::device_t *sata_controller = PCI::get_pci_dev(0x1, 0x6, 0x1);
    hba_memory = (hba_mem_t*)((uint64_t)sata_controller->BARS[5]);
    kernel::map_page((uint64_t)hba_memory, (uint64_t)hba_memory);
    probe_ports();
}

}