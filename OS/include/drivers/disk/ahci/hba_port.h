#pragma once
#include "drivers/disk/ahci/ahci_cmd.h"
#include <types.h>

namespace disk
{
    struct sata_status_t
    {
        uint32_t device_detection : 4; // aka: DET
        uint32_t current_speed : 4;    // aka: SPD
        uint32_t interface_pm : 4;     // aka: IPM
        uint32_t rsv0 : 20;            // reserved
    } __attribute__((packed));

    enum hba_port_signature_t : uint32_t
    {
        SIGNATURE_ATA = 0x00000101,
        SIGNATURE_ATAPI = 0xEB140101,
        SIGNATURE_SEMB = 0xC33C0101,
        SIGNATURE_PM = 0x96690101,
    };

    struct interrupt_sts_t
    {
        uint32_t device_to_host_reg_FIS_intr : 1;
        uint32_t PIO_setup_FIS_intr : 1;
        uint32_t DMA_setup_FIS_intr : 1;
        uint32_t set_dev_bit_intr : 1;
        uint32_t unknown_FIS_intr : 1;
        uint32_t descriptor_processed : 1;
        uint32_t port_connect_change_status : 1;
        uint32_t device_mechanical_presense_sts : 1;
        uint32_t rsv0 : 14;
        uint32_t phyrdy_change_sts : 1;
        uint32_t incorrect_port_multiplier_sts : 1;
        uint32_t overflow_status : 1;
        uint32_t rsv1 : 1;
        uint32_t interface_non_fatal_error_sts : 1;
        uint32_t interface_fatal_error_sts : 1;
        uint32_t host_bus_data_error_sts : 1;
        uint32_t host_bus_fatal_error_sts : 1;
        uint32_t task_file_error_sts : 1;
        uint32_t cold_port_detect_sts : 1;
    } __attribute__((packed));

    struct interrupt_enable_t
    {
        uint32_t device_to_host_reg_FIS_intr_enable : 1;
        uint32_t PIO_setup_FIS_intr_enable : 1;
        uint32_t DMA_setup_FIS_intr_enable : 1;
        uint32_t set_dev_bit_intr_enable : 1;
        uint32_t unknown_FIS_intr_enable : 1;
        uint32_t descriptor_processed_intr_enable : 1;
        uint32_t port_connect_change_intr_enable : 1;
        uint32_t device_mechanical_presense_enable : 1;
        uint32_t rsv0 : 14;
        uint32_t phyrdy_change_enable : 1;
        uint32_t incorrect_port_multiplier_enable : 1;
        uint32_t overflow_enable : 1;
        uint32_t rsv1 : 1;
        uint32_t interface_non_fatal_error_enable : 1;
        uint32_t interface_fatal_error_enable : 1;
        uint32_t host_bus_data_error_enable : 1;
        uint32_t host_bus_fatal_error_enable : 1;
        uint32_t task_file_error_enable : 1;
        uint32_t cold_presence_detect_enable : 1;

    } __attribute__((packed));

    struct command_status_t
    {
        uint32_t cmd_start : 1;
        uint32_t spin_up_device : 1;
        uint32_t power_on_device : 1;
        uint32_t cmd_list_override : 1;
        uint32_t FIS_recieve_enable : 1;
        uint32_t rsv0 : 3;
        uint32_t current_cmd_slot : 5;
        uint32_t mechanical_presence_switch_state : 1;
        uint32_t FIS_recieve_running : 1;
        uint32_t cmd_list_running : 1;
        uint32_t cold_presence_state : 1;
        uint32_t port_multiplier_attached : 1;
        uint32_t hot_plug_capable_port : 1;
        uint32_t mechanical_presence_switch_attached : 1;
        uint32_t cold_presence_detection : 1;
        uint32_t external_sata_port : 1;
        uint32_t FIS_based_switching_port : 1;
        uint32_t auto_partial_slumber_transitions_enabled : 1;
        uint32_t device_is_ATAPI : 1;
        uint32_t drive_led_on_ATAPI_enable : 1;
        uint32_t aggressive_link_power_mgmt_enable : 1;
        uint32_t aggressive_slumber : 1;
        uint32_t interface_communication_ctrl : 4;
    } __attribute__((packed));

    struct task_file_data_t
    {
        uint8_t status_err : 1;
        uint8_t status_command_specific : 2;
        uint8_t status_data_transfer_requested : 1;
        uint8_t status_command_specific_2 : 3;
        uint8_t status_busy : 1;
        uint8_t error;
        uint16_t rsv0;
    } __attribute__((packed));

    struct sata_ctrl_t
    {
        uint32_t device_detection_init : 4;
        uint32_t speed_allowed : 4;
        uint32_t ipm_transitions_allowed : 4;
        uint32_t rsv0 : 20;
    } __attribute__((packed));

    struct sata_err_t
    {
        struct err_diagnostics
        {
            uint16_t phyrdy_change : 1;
            uint16_t phy_internal_err : 1;
            uint16_t comm_wake : 1;
            uint16_t decode_err : 1;
            uint16_t disparity_err : 1;
            uint16_t crc_err : 1;
            uint16_t handshke_err : 1;
            uint16_t link_sequence_err : 1;
            uint16_t transport_state_transition_error : 1;
            uint16_t unknown_FIS_type : 1;
            uint16_t exchanged : 1;
            uint16_t rsv0 : 5;
        } __attribute__((packed)) diagnostics;

        struct error_type
        {
            uint16_t recovered_data_integrity_err : 1;
            uint16_t recovered_comms_err : 1;
            uint16_t rsv0 : 6;
            uint16_t transient_data_integrity_err : 1;
            uint16_t persistent_comms_or_data_integrity_err : 1;
            uint16_t protocol_err : 1;
            uint16_t internal_err : 1;
            uint16_t rsv1 : 4;
        } __attribute__((packed)) error;

    } __attribute__((packed));

    struct fis_switching_ctrl_t
    {
        uint32_t enable : 1;
        uint32_t device_err_clear : 1;
        uint32_t single_device_err : 1;
        uint32_t rsv0 : 5;
        uint32_t device_to_issue : 4;
        uint32_t active_device_optimization : 4;
        uint32_t device_with_err : 4;
        uint32_t rsv1 : 12;
    } __attribute__((packed));

    struct device_sleep_t
    {
        uint32_t aggressive_device_sleep_enable : 1;
        uint32_t device_sleep_present : 1;
        uint32_t device_sleep_exit_timeout : 8;
        uint32_t minimum_device_sleep_assertion_time : 5;
        uint32_t device_sleep_idle_timeout : 10;
        uint32_t DITO_multiplier : 4;
        uint32_t rsv0 : 3;
    } __attribute__((packed));

    struct hba_port_t
    {
        command_hdr_t *cmd_list_base_addr;
        HBA_FIS *fis_base_addr;
        interrupt_sts_t interupt_status;
        interrupt_enable_t interrupt_enable;
        command_status_t command_status;
        uint32_t rsv0;
        task_file_data_t task_file_data;
        hba_port_signature_t signature;
        sata_status_t sata_status;
        sata_ctrl_t sata_control;
        sata_err_t sata_error;
        uint32_t sata_active;
        uint32_t command_issue;
        uint32_t sata_notification;
        fis_switching_ctrl_t fis_switching_ctrl;
        device_sleep_t device_sleep;
        uint8_t rsv1[40];
        uint8_t vendor_regs[16];
    } __attribute__((packed));

}