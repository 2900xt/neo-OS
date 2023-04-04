#pragma once
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

    AHCIDevice(uint8_t port_number);

    int read(uint64_t starting_lba, uint32_t sector_cnt, void *dma_buffer);

protected:

    int getSlot();
    int identifyDevice();

};

}