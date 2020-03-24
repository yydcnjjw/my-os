#pragma once
#include <my-os/list.h>

struct pci_device_config {
    u16 vendor_id;
    u16 device_id;
    u16 command;
    u16 status;
    u8 revision_id;
    u8 prog_if;
    u8 sub_class;
    u8 class_code;
    u8 cache_line_size;
    u8 latency_timer;
    u8 header_type;
    u8 bist;

    union {
        // header type 00h
        struct {
            u32 bar0;
            u32 bar1;
            u32 bar2;
            u32 bar3;
            u32 bar4;
            u32 bar5;
            u32 cardbus_cis_ptr;
            u16 subsystem_vendor_id;
            u16 subsystem_id;
            u32 expansion_rom_base_addr;
            u32 capabilities_ptr;
            u32 reserved[3];
            u8 interrupt_line;
            u8 interrupt_pin;
            u8 min_grant;
            u8 max_latency;
        };
        // header type 01h
    };
};

struct pci_device {
    u8 bus;
    u8 device;
    u8 function;
    struct pci_device_config config;
    struct list_head list;
};

extern struct list_head pci_devices;
