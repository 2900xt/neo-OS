#include <drivers/acpi/sdt.h>
#include <stdlib/stdlib.h>

volatile struct limine::limine_rsdp_request const rsdp_request = { LIMINE_RSDP_REQUEST, 0 };

RSDPDescriptor* rsdp;

void const* fadt;

RSDPDescriptor* getRSDP(void)
{
    if(rsdp != NULL){
        return rsdp;
    }

    rsdp = (RSDPDescriptor*)rsdp_request.response->address;

    klogf(LOG_DEBUG, "RSDP at -> 0x%x\n", rsdp);

    return rsdp;
}



void* findACPITable(char *const signature)
{
	size_t const len = strlen(signature);
	ACPI_XSDT *const xsdt = getRSDP()->XSDTAddress;
    const int entryCount = (xsdt->hdr.length - sizeof(xsdt->hdr)) / 8;

    for(int i = 0; i < entryCount; i++){
      ACPI_SDT_HEADER *const h = (ACPI_SDT_HEADER*) xsdt->ptr[i];
        if(strcmp(h->signature, signature, len)){         
           return (void*)h; 
        }
    }

    return nullptr;
}
