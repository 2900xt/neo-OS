#include "stdlib/stdio.h"
#include <drivers/acpi/sdt.h>
#include <stdlib/stdlib.h>

namespace ACPI 
{

volatile struct limine::limine_rsdp_request rsdp_request = { LIMINE_RSDP_REQUEST, 0};

static RSDPDescriptor* rsdp;

RSDPDescriptor* getRSDP(void)
{
    if(rsdp != NULL){
        return rsdp;
    }

    rsdp = (RSDPDescriptor*)rsdp_request.response->address;
    return rsdp;
}



void* findACPITable(const char* signature)
{
	size_t const len = std::strlen(signature);
	XSDT *const xsdt = getRSDP()->XSDTAddress;
    const int entryCount = (xsdt->hdr.length - sizeof(xsdt->hdr)) / 8;

    for(int i = 0; i < entryCount; i++){
        SDT_HEADER *const h = (SDT_HEADER*) xsdt->ptr[i];
        if(std::strcmp(h->signature, signature, len)){         
           return (void*)h; 
        }
    }

    return NULL;
}

}