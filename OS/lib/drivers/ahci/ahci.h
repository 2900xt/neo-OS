#pragma once
#include "drivers/ahci/ahci_cmd.h"
#include "drivers/ahci/hba_fis.h"
#include <types.h>
#include <drivers/ahci/hba_ghc.h>
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

class AHCIDevice 
{

public:

    hba_port_t      *port;
    uint8_t          port_num;
    HBA_CMD_HEADER  *cmd_list;
    HBA_FIS         *port_fis;

    AHCIDevice(uint8_t port_number);

};

}