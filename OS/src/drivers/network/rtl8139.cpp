#include <stdlib/stdlib.h>
#include <kernel/kernel.h>
#include <drivers/pci/pci.h>
#include <drivers/network/rtl8139.h>

namespace network
{
    const char *rtl8139_tag = "RTL8139";

    void rtl8139_init()
    {
        pci::device_t *dev = pci::get_pci_dev(0x10EC, 0x8139);
        if (dev == NULL)
        {
            log::e(rtl8139_tag, "RTL8139 network controller not found");
            return;
        }

        dev->hdr.command.bus_master_enable = 1;
        dev->hdr.command.mem_space_enable = 1;
        dev->hdr.command.io_space_enable = 1;
        dev->hdr.command.intr_disable = 1;

        log::d(rtl8139_tag, "RTL8139 network controller initialized");
    }
}