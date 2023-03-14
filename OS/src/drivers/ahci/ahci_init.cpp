#include "drivers/ahci/hba_port.h"
#include "stdlib/stdio.h"
#include "types.h"
#include <stdlib/stdlib.h>
#include <kernel/mem/paging.h>
#include <drivers/ahci/ahci.h>
#include <drivers/pci/pci.h>

namespace AHCI {

hba_mem_t *hba_memory;
uint8_t port_count;

static void probe_ports()
{
    assert(hba_memory != NULL);
    port_count = 0;

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
                default:
                    ok = false;
                    break;
            }

            if(!ok) continue;

            if(hba_memory->ports[i].sata_status.device_detection != 0x3 || hba_memory->ports[i].sata_status.interface_pm != 0x1) continue;

            port_count++;
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
