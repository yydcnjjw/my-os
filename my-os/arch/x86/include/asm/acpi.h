#pragma once

#include <my-os/types.h>

struct RSDPDescriptor {
    char signature[8];
    u8 checksum;
    char OEMID[6];
    u8 revision;
    u32 rsdt_address;
} __attribute__((packed));

struct SDTHeader {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char OEM_id[6];
    char OEM_table_id[8];
    u32 OEM_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__((packed));

struct RSDTDescriptor {
    struct SDTHeader header;
    u32 other_sdt[0];
};

extern struct RSDTDescriptor *rsdt;
void acpi_init();
