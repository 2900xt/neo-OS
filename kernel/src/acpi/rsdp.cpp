#include <acpi/tables.h>
#include <stdout.h>
#include <mem.h>

volatile struct limine::limine_rsdp_request rsdp_request = {LIMINE_RSDP_REQUEST, 0};

RSDPDescriptor* rsdp;

void* fadt;

RSDPDescriptor* getRSDP(void)
{
    if(rsdp != NULL){
        return rsdp;
    }

    rsdp = (RSDPDescriptor*)rsdp_request.response->address;

    klogf(LOG_DEBUG, "RSDP at -> 0x%x\n", rsdp);

    return rsdp;
}



void* findACPITable(char* signature)
{
    size_t len                = strlen(signature);
	ACPI_XSDT *const xsdt = getRSDP()->XSDTAddress;
    int entryCount            = (xsdt->hdr.length - sizeof(xsdt->hdr)) / 8;

    for(int i = 0; i < entryCount; i++){
        ACPI_SDT_HEADER* h = (ACPI_SDT_HEADER*)xsdt->ptr[i];
        if(strcmp(h->signature, signature, len)){         
           return (void*)h; 
        }
    }

    return NULL;
}
