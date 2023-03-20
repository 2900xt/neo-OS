#include "drivers/ahci/ahci_cmd.h"
#include "drivers/ahci/hba_fis.h"
#include "drivers/ahci/hba_port.h"
#include <types.h>
#include <drivers/ahci/ahci.h>
#include <stdlib/stdio.h>
#include <kernel/mem/paging.h>
#include <kernel/mem/mem.h>
#include <kernel/vfs/file.h>
#include <stdlib/string.h>


namespace AHCI {

extern hba_mem_t *hba_memory;
extern uint8_t device_count;
extern AHCIDevice* devices[32];

static int hd_cnt = 0, cd_cnt = 0;

// class AHCIDevice

AHCIDevice::AHCIDevice(uint8_t port_num)
{
    this->port_num = port_num;
    this->port = &hba_memory->ports[port_num];

    //Add the block device

    char* new_filename = (char*)kcalloc(1, 5);
    if(port->signature == SIGNATURE_ATA)
    {
        std::strcpy(new_filename, "hd");
        std::strcat(new_filename, std::itoa(hd_cnt++, 10));
    }
    else {
        std::strcpy(new_filename, "cdrom");
        std::strcat(new_filename, std::itoa(cd_cnt++, 10));
    }

    VFS::file_t* block = VFS::get_root()->get_subdir("dev")->create_child(new_filename, VFS::DEVICE);
    block->file_data = (void*)this;
    block->permissions = 0xFF;

    //Get 3 * 4K pages for data
    //Then map them into virtual memory

    uint64_t page = (uint64_t)kernel::allocate_pages(3);
    kernel::map_pages(page, page + 0x3000, page, page + 0x3000);
}
}


