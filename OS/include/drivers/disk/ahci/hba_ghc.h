#pragma once
#include <types.h>

namespace disk
{
    struct host_capabillities_t
    {
        uint32_t number_of_ports : 5;            // aka: NP
        uint32_t supports_external_sata : 1;     // aka: SXS
        uint32_t encolsure_mgmt_supported : 1;   // aka: EMS
        uint32_t ccc_suported : 1;               // aka: CCCS
        uint32_t number_of_cmd_slots : 5;        // aka: NCS
        uint32_t partial_state_capabile : 1;     // aka: PSC
        uint32_t slumber_state_capable : 1;      // aka: SSC
        uint32_t pio_multiple_drq_blk : 1;       // aka: PMD
        uint32_t fis_based_switch_support : 1;   // aka: FBSS
        uint32_t supports_port_multiplier : 1;   // aka: SPM
        uint32_t supports_ahci_mode_only : 1;    // aka: SAM
        uint32_t rsv0 : 1;                       // reserved
        uint32_t interface_speed_support : 4;    // aka: ISS
        uint32_t support_cmd_list_overrride : 1; // aka: SCLO
        uint32_t support_activity_led : 1;       // aka: SAL
        uint32_t support_alpm : 1;               // aka: SALP
        uint32_t support_sss : 1;                // aka: SSS
        uint32_t support_mps : 1;                // aka: SMPS
        uint32_t support_snotification_reg : 1;  // aka: SSNTF
        uint32_t support_native_cmd_queuing : 1; // aka: SNCQ
        uint32_t support_64_bit_addressing : 1;  // aka: S64A
    } __attribute__((packed));

    struct global_host_ctrl_t
    {
        uint32_t hba_reset : 1;   // aka: HR
        uint32_t intr_enable : 1; // aka: IE
        uint32_t mrsm : 1;        // aka: MSRM
        uint32_t rsv0 : 28;       // reserved
        uint32_t ahci_enable : 1; // aka: AE
    } __attribute__((packed));

    struct ccc_control_t
    {
        uint32_t enable : 1;              // aka: EN
        uint32_t rsv0 : 2;                // reserved
        uint32_t interrupt : 5;           // aka: INT
        uint32_t command_completions : 8; // aka: CC
        uint32_t timeout_val : 16;        // aka: TV
    } __attribute__((packed));

    struct encolsure_mgmt_loc_t
    {
        uint16_t offset;
        uint16_t buffer_size;
    } __attribute__((packed));

    struct encolsure_mgmt_ctrl_t
    {
        uint32_t message_recieved : 1;
        uint32_t rsv0 : 7;
        uint32_t transmit_message : 1;
        uint32_t reset : 1;
        uint32_t rsv1 : 6;
        uint32_t led_message_types : 1;
        uint32_t SAF_TE_messages : 1;
        uint32_t SES_2_messages : 1;
        uint32_t SGPIO_messages : 1;
        uint32_t rsv2 : 4;
        uint32_t single_message_buffer : 1;
        uint32_t transmit_only : 1;
        uint32_t activity_led : 1;
        uint32_t port_multiplier_support : 1;
        uint32_t rsv3 : 4;
    } __attribute__((packed));

    struct hba_cap_ext_t
    {
        uint32_t bios_handoff : 1;
        uint32_t NVMHCI_present : 1;
        uint32_t auto_partial_to_slumber : 1;
        uint32_t support_dev_sleep : 1;
        uint32_t support_aggressive_dev_sleep : 1;
        uint32_t dev_sleep_entrance_from_slumber : 1;
        uint32_t rsv0 : 26;
    } __attribute__((packed));

    struct bios_handoff_t
    {
        uint32_t BIOS_owned_semaphore : 1;
        uint32_t OS_owned_semaphore : 1;
        uint32_t SMI_on_OS_ownership_change_enabled : 1;
        uint32_t OS_ownership_change : 1;
        uint32_t BIOS_busy : 1;
        uint32_t rsv0 : 27;
    } __attribute__((packed));

    struct generic_host_ctrl
    {
        host_capabillities_t host_cap;
        global_host_ctrl_t g_host_ctrl;
        uint32_t interrupt_status;
        uint32_t ports_implemented;
        uint32_t hba_version;
        ccc_control_t ccc_control;
        uint32_t ccc_ports;
        encolsure_mgmt_loc_t enclosure_mgmt_loc;
        encolsure_mgmt_ctrl_t enclosure_mgmt_ctrl;
        hba_cap_ext_t host_capabillities_ext;
        bios_handoff_t bios_handoff_ctrl_sts;
    } __attribute__((packed));

} // AHCI