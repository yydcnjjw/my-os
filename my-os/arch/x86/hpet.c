#include <my-os/types.h>

struct address_structure {
    u8 address_space_id; // 0 - system memory, 1 - system I/O
    u8 register_bit_width;
    u8 register_bit_offset;
    u8 reserved;
    u64 address;
} __attribute__((packed));

struct description_table_header {
    char signature[4]; // 'HPET' in case of HPET table
    u32 length;
    u8 revision;
    u8 checksum;
    char oemid[6];
    u64 oem_tableid;
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__((packed));

struct hpet {
    struct description_table_header header;
    u8 hardware_rev_id;
    u8 comparator_count : 5;
    u8 counter_size : 1;
    u8 reserved : 1;
    u8 legacy_replacement : 1;
    u16 pci_vendor_id;
    struct address_structure address;
    u8 hpet_number;
    u16 minimum_tick;
    u8 page_protection;
} __attribute__((packed));
