#pragma once
#include <types.h>

struct phy_region_desc
{
    uint64_t data_base_address; //DBA
    uint32_t rsv0;
    uint32_t data_byte_count : 22;
    uint32_t rsv1 : 9;
    uint32_t intr_on_completion : 1;
}__attribute__((packed));

struct command_tbl_t
{
    uint8_t cmd_fis[64];
    uint8_t atapi_cmd[16];
    uint8_t rsv0[48];
    phy_region_desc prdt[8];
};

struct command_hdr_t
{
    //DWORD 0
    uint32_t cmdfis_length : 5; //CFL
    uint32_t ATAPI_command : 1; //A
    uint32_t write_command : 1; //W
    uint32_t prefetchable : 1;  //P
    uint32_t reset : 1;         //R
    uint32_t BIST : 1;          //B
    uint32_t clear_busy : 1;    //C
    uint32_t rsv0 : 1;
    uint32_t port_mult : 4;     //PMP
    uint32_t prdt_length : 16;  //PRDTL

    //DWORD 1
    volatile uint32_t prdt_byte_count;   //PRDBC

    //DWORD 2-3
    command_tbl_t* cmd_table_desc_base_addr; //CTBA

    uint32_t rsv1[4];
}__attribute__((packed));

struct FIS_REG_H2D
{
    //DWORD 0
    uint8_t fis_type;
    uint8_t pmport : 4;
    uint8_t rsv0 : 3;
    uint8_t c : 1;

    uint8_t command;
    uint8_t feature_low;
    
    //DWORD 1

    uint8_t lba0, lba1, lba2;
    uint8_t device;

    //DWORD 2

    uint8_t lba3, lba4, lba5;
    uint8_t feature_high;

    //DWORD 3

    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t ctrl;

    //DWORD 4
    uint32_t rsv1;

}__attribute__((packed));


struct FIS_REG_D2H
{
    //DWORD 0
    uint8_t fis_type;
    uint8_t pmport : 4;
    uint8_t rsv0 : 2;
    uint8_t i : 1;
    uint8_t c : 1;

    uint8_t status;
    uint8_t error;
    
    //DWORD 1

    uint8_t lba0, lba1, lba2;
    uint8_t device;

    //DWORD 2

    uint8_t lba3, lba4, lba5;
    uint8_t rsv1;

    //DWORD 3

    uint8_t countl;
    uint8_t counth;
    uint8_t rsv2[2];

    //DWORD 4
    uint32_t rsv3;
}__attribute__((packed));

struct FIS_DATA
{
    //DWORD 0

    uint8_t fis_type;
    uint8_t pmport : 4;
    uint8_t rsv0 : 4;
    uint8_t rsv1[2];

    //Data can have a variable size

    uint8_t data[1];
}__attribute__((packed));

struct FIS_DMA_SETUP
{
    //DWORD 0
    uint8_t fis_type;
    uint8_t rsv0 : 1;
    uint8_t d : 1;
    uint8_t i : 1;
    uint8_t a : 1;
    uint8_t rsv1[2];

    //DWORD 1-2
    uint64_t DMA_buffer_id;

    //DWORD 3
    uint32_t rsv2;

    //DWORD 4
    uint32_t DMA_buffer_offset;

    //DWORD 5
    uint32_t transfer_cnt;
    
    //DWORD 6
    uint32_t rsv3;
}__attribute__((packed));

struct FIS_PIO_SETUP
{
   //DWORD 0
    uint8_t fis_type;
    uint8_t pmport : 4;
    uint8_t rsv0 : 1;
    uint8_t d : 1;
    uint8_t i : 1;
    uint8_t rsv1 : 1;

    uint8_t status;
    uint8_t error;
    
    //DWORD 1

    uint8_t lba0, lba1, lba2;
    uint8_t device;

    //DWORD 2

    uint8_t lba3, lba4, lba5;
    uint8_t rsv2;

    //DWORD 3

    uint8_t countl;
    uint8_t counth;
    uint8_t rsv3;
    uint8_t e_status;

    //DWORD 4
    uint16_t transfer_cnt;
    uint8_t rsv4[2];
}__attribute__((packed));

struct HBA_FIS
{
    FIS_DMA_SETUP dma_setup_fis;
    uint8_t rsv0[4];

    FIS_PIO_SETUP pio_setup_fis;
    uint8_t rsv1[12];

    FIS_REG_D2H reg_d2h_fis;
    uint8_t rsv2[8];

    uint8_t ufis[64];

    uint8_t rsv3[0x100 - 0xA0];

}__attribute__((packed));

typedef enum
{
	FIS_TYPE_REG_H2D	= 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H	= 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT	= 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP	= 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA		= 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST		= 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP	= 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS	= 0xA1,	// Set device bits FIS - device to host
} FIS_TYPE;