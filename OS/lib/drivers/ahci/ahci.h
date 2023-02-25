#pragma once
#include <types.h>

namespace AHCI {

struct host_capabillities_t
{
    uint32_t number_of_ports : 5;           //aka: NP
    uint32_t supports_external_sata : 1;    //aka: SXS
    uint32_t encolsure_mgmt_supported : 1;  //aka: EMS
    uint32_t ccc_suported : 1;              //aka: CCCS
    uint32_t number_of_cmd_slots : 5;       //aka: NCS
    uint32_t partial_state_capabile : 1;    //aka: PSC
    uint32_t slumber_state_capable : 1;     //aka: SSC
    uint32_t pio_multiple_drq_blk : 1;      //aka: PMD
    uint32_t fis_based_switch_support : 1;  //aka: FBSS
    uint32_t supports_port_multiplier : 1;  //aka: SPM
    uint32_t supports_ahci_mode_only : 1;   //aka: SAM
    uint32_t rsv0 : 1;                      //reserved
    uint32_t interface_speed_support : 4;   //aka: ISS
    uint32_t support_cmd_list_overrride : 1;//aka: SCLO
    uint32_t support_activity_led : 1;      //aka: SAL
    uint32_t support_alpm : 1;              //aka: SALP
    uint32_t support_sss : 1;               //aka: SSS
    uint32_t support_mps : 1;               //aka: SMPS
    uint32_t support_snotification_reg : 1; //aka: SSNTF
    uint32_t support_native_cmd_queuing : 1;//aka: SNCQ
    uint32_t support_64_bit_addressing : 1; //aka: S64A
}__attribute__((packed));

struct global_host_ctrl_t
{
    uint32_t hba_reset : 1;     //aka: HR
    uint32_t intr_enable : 1;   //aka: IE
    uint32_t mrsm : 1;          //aka: MSRM
    uint32_t rsv0 : 28;         //reserved
    uint32_t ahci_enable : 1;   //aka: AE
}__attribute__((packed));

struct ccc_control_t
{
    uint32_t enable : 1;                //aka: EN
    uint32_t rsv0 : 2;                  //reserved
    uint32_t interrupt : 5;             //aka: INT
    uint32_t command_completions: 8;    //aka: CC
    uint32_t timeout_val : 16;          //aka: TV
}__attribute__((packed));

struct sata_status_t
{
    uint32_t device_detection : 4;      //aka: DET
    uint32_t current_speed : 4;         //aka: SPD
    uint32_t interface_pm : 4;          //aka: IPM
    uint32_t rsv0 : 20;                 //reserved
};

struct generic_host_ctrl
{
    host_capabillities_t host_cap;              //aka: CAP
    global_host_ctrl_t global_host_ctrl;        //aka: GHC
    uint32_t interrupt_status;                  //aka: IS
    uint32_t ports_implemented;                 //aka: PI
    uint32_t hba_version;                       //aka: VS
    ccc_control_t ccc_control;                  //aka: CCC_CTL
    uint32_t ccc_ports;                         //aka: CCC_PORTS
    uint32_t enclosure_mgmt_loc;                //aka: EM_LOC
    uint32_t enclosure_mgmt_ctrl;               //aka: EM_CTL
    uint32_t host_capabillities_ext;            //aka: CAP2
    uint32_t bios_handoff_ctrl_sts;             //aka: BOHC
}__attribute__((packed));

struct hba_port_t
{
    uint64_t cmd_list_base;
    uint64_t fis_base_addr;
    uint32_t interupt_status;
    uint32_t interrupt_enable;
    uint32_t command_status;
    uint32_t rsv0;
    uint32_t task_file_data;
    uint32_t signature;
    sata_status_t sata_status;
    uint32_t sata_control;
    uint32_t sata_error;
    uint32_t sata_active;
    uint32_t command_issue;
    uint32_t sata_notification;
    uint32_t fis_switching_ctrl;
    uint8_t rsv1[44];
    uint8_t vendor_regs[16];
}__attribute__((packed));

enum hba_port_signature_t : uint32_t
{
    SIGNATURE_ATA   = 0x00000101,
    SIGNATURE_ATAPI = 0xEB140101,
    SIGNATURE_SEMB  = 0xC33C0101,
    SIGNATURE_PM    = 0x96690101,
};

struct hba_mem_t
{
    generic_host_ctrl ghc;
    uint8_t rsv0[52];
    uint8_t nvmhci[64];
    uint8_t vendor_regs[96];
    hba_port_t ports[32];
}__attribute__((packed));

struct port_t
{
    uint8_t port_type;
    uint8_t port_number;
    hba_port_t *port;
    uint8_t *dma_buffer;
};

struct hba_command_hdr_t
{
    uint8_t command_fis_length : 5;
    uint8_t atapi : 1;
    uint8_t write : 1;
    uint8_t prefetchable : 1;
    uint8_t reset : 1;
    uint8_t bist : 1;
    uint8_t clear_busy : 1;
    uint8_t rsv0 : 1;
    uint8_t port_multiplier : 4;
    uint16_t prdt_length;
    uint32_t prdb_count;
    uint64_t cmd_table_base_addr;
    uint32_t rsv1[4];
}__attribute__((packed));

struct fis_reg_h2d_t
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;
    uint8_t rsv0 : 3;
    uint8_t command_ctrl : 1;
    uint8_t command;

    uint8_t feature_low;

    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;

    uint8_t device_register;

    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;

    uint8_t feature_high;

    uint16_t count;
    uint8_t iso_command_completion;
    uint8_t control;
    uint8_t rsv1[4];

}__attribute__((packed));

struct hba_prdt_entry_t
{
    uint64_t data_base_addr;
    uint32_t rsv0;
    uint32_t byte_count : 22;
    uint32_t rsv1 : 9;
    uint32_t interrupt_on_completion : 1;

}__attribute__((packed));

struct hba_cmd_table_t
{
    uint8_t command_fis[64];
    uint8_t atapi_cmd[16];
    uint8_t rsv0[48];
    hba_prdt_entry_t prdtEntries[];

}__attribute__((packed));

void ahci_init();

}