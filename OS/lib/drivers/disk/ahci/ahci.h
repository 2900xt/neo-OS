#pragma once
#include <types.h>
#include <drivers/disk/ahci/hba_ghc.h>
#include <drivers/disk/ahci/hba_port.h>
#include "drivers/fs/gpt.h"

namespace DISK {

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

    FS::gpt_part_data   *gpt_data;
    hba_port_t          *port;
    uint8_t             port_num;

    AHCIDevice(uint8_t port_number);

    int read(uint64_t starting_lba, uint32_t sector_cnt, void *dma_buffer);
    int write(uint64_t starting_lba, uint32_t sector_cnt, void *data_buffer);
    int identifyDevice(uint16_t *buffer);

    FS::gpt_part_data *get_gpt();
    
protected:

    int getSlot();
    int run_command(int slot);
    void read_gpt();

};

}