#include "drivers/disk/ahci/hba_port.h"
#include "stdlib/stdio.h"
#include "types.h"
#include <stdlib/stdlib.h>
#include <kernel/mem/paging.h>
#include <drivers/disk/ahci/ahci.h>
#include <drivers/pci/pci.h>
#include <drivers/disk/ahci/ahci_cmd.h>
namespace DISK {

hba_mem_t *hba_memory;
uint8_t device_count;
AHCIDevice* devices[32];

static void probe_ports()
{
    assert(hba_memory != NULL);
    device_count = 0;

    for(int i = 0; i < 32; i++)
    {
        if(hba_memory->ghc.ports_implemented & (1 << i))
        {
            bool ok = false;
            switch(hba_memory->ports[i].signature)
            {
                case SIGNATURE_ATA:
                    ok = true;
                    break;
                default:
                    ok = false;
                    break;
            }

            if(!ok) continue;

            if(hba_memory->ports[i].sata_status.device_detection != 0x3 || hba_memory->ports[i].sata_status.interface_pm != 0x1) continue;

            devices[i] = new AHCIDevice(i);

            device_count++;
        }
    }
}

void ahci_init()
{
    PCI::device_t *sata_controller = PCI::get_pci_dev(0x1, 0x6, 0x1);

    sata_controller->hdr.command.intr_disable = 0;
    sata_controller->hdr.command.mem_space_enable = 1;

    hba_memory = (hba_mem_t*)((uint64_t)sata_controller->BARS[5] & ~0xFFF);
    kernel::map_page((uint64_t)hba_memory, (uint64_t)hba_memory);
    probe_ports();

}

}
