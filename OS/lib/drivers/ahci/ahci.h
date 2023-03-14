#pragma once
#include <types.h>
#include <drivers/ahci/generic_host_control.h>
#include <drivers/ahci/hba_port.h>

namespace AHCI {

struct hba_mem_t
{
    generic_host_ctrl ghc;
    uint8_t rsv0[52];
    uint8_t nvmhci[64];
    uint8_t vendor_regs[96];
    hba_port_t ports[32];
}__attribute__((packed));



void ahci_init();

}
