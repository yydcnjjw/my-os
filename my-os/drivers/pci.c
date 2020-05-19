#include <asm/io.h>
#include <kernel/printk.h>
#include <my-os/kernel.h>
#include <my-os/list.h>
#include <my-os/pci.h>
#include <my-os/slub_alloc.h>
#include <my-os/string.h>
#include <my-os/types.h>

LIST_HEAD(pci_devices);

#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc
#define PCI_VENDOR_ID 0x2
#define PCI_HEADER_TYPE 0xe

void pci_config_set_addr(u8 bus, u8 device, u8 func, u8 offset) {
    u32 address;
    u32 lbus = (u32)bus;
    u32 ldevice = (u32)device;
    u32 lfunc = (u32)func;

    address = (u32)((lbus << 16) | (ldevice << 11) | (lfunc << 8) |
                    (offset & 0xfc) | ((u32)0x80000000));

    outl(address, PCI_CONFIG_ADDRESS);
}

u32 pci_config_readl(u8 bus, u8 device, u8 func, u8 offset) {
    pci_config_set_addr(bus, device, func, offset);
    return inl(PCI_CONFIG_DATA);
}
u16 pci_config_readw(u8 bus, u8 device, u8 func, u8 offset) {
    u32 value = pci_config_readl(bus, device, func, offset);
    return (u16)((value >> ((offset & 2) * 8)) & 0xffff);
}

u8 pci_config_readb(u8 bus, u8 device, u8 func, u8 offset) {
    u32 value = pci_config_readl(bus, device, func, offset);
    return (u8)((value >> ((offset & 3) * 8)) & 0xff);
}
void pci_config_read(u8 bus, u8 device, u8 function, void *dst, size_t size) {
    kassert(size < 256);
    kassert(ALIGN(size, 4));
    for (size_t i = 0; i < size; i += 4) {
        u32 v = pci_config_readl(bus, device, function, i);
        memcpy(dst, &v, sizeof(v));
        dst = (u32 *)dst + 1;
    }
}

void pci_config_writeb(u8 bus, u8 device, u8 function, u8 offset, u8 v) {
    pci_config_set_addr(bus, device, function, offset);
    outb(v, PCI_CONFIG_DATA);
}

void pci_config_writew(u8 bus, u8 device, u8 function, u8 offset, u16 v) {
    pci_config_set_addr(bus, device, function, offset);
    outb(v, PCI_CONFIG_DATA);
}
void pci_config_writel(u8 bus, u8 device, u8 function, u8 offset, u32 v) {
    pci_config_set_addr(bus, device, function, offset);
    outb(v, PCI_CONFIG_DATA);
}

void register_pci_device(u8 bus, u8 device, u8 function) {
    struct pci_device *pci_device =
        kmalloc(sizeof(struct pci_device), SLUB_NONE);
    pci_device->bus = bus;
    pci_device->device = device;
    pci_device->function = function;
    pci_config_read(bus, device, function, &pci_device->config,
                    sizeof(struct pci_device_config));

    list_add(&pci_device->list, &pci_devices);
}

bool pci_check_device(u8 bus, u8 device) {
    u16 vendor_id = pci_config_readw(bus, device, 0, PCI_VENDOR_ID);
    if (vendor_id == 0xffff)
        return false;

    register_pci_device(bus, device, 0);

    u8 header_type = pci_config_readb(bus, device, 0, PCI_HEADER_TYPE);

    if ((header_type & 0x80) != 0) {
        for (int function = 1; function < 8; function++) {
            if (pci_config_readw(bus, device, function, PCI_VENDOR_ID) !=
                0xffff) {
                register_pci_device(bus, device, function);
            }
        }
    }
    return true;
}

void pci_check_all_buses(void) {
    for (u16 bus = 0; bus < 256; bus++) {
        for (u8 device = 0; device < 32; device++) {
            pci_check_device(bus, device);
        }
    }
}

struct pci_device *get_pci_device(u8 class_code, u8 sub_class) {
    struct pci_device *pci_device;
    list_for_each_entry(pci_device, &pci_devices, list) {
        if (pci_device->config.class_code == class_code &&
            pci_device->config.sub_class == sub_class) {
            break;
        }
    }
    return pci_device;
}

void pci_bus() { pci_check_all_buses(); }
