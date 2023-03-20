#include <drivers/pci/pci.h>
#include <drivers/ahci/ahci.h>

namespace kernel
{

void load_drivers()
{
    PCI::enumerate_pci();
    AHCI::ahci_init();
}

}