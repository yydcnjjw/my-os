#include <asm/idt.h>
#include <asm/io.h>
#include <kernel/printk.h>

#include <my-os/pci.h>
#include <my-os/string.h>

#include "identify_device_data.h"

#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX 0x02
#define ATA_SR_ERR 0x01

#define ATA_ER_BBK 0x80
#define ATA_ER_UNC 0x40
#define ATA_ER_MC 0x20
#define ATA_ER_IDNF 0x10
#define ATA_ER_MCR 0x08
#define ATA_ER_ABRT 0x04
#define ATA_ER_TK0NF 0x02
#define ATA_ER_AMNF 0x01

// ATA-Commands:
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_READ_PIO_EXT 0x24
#define ATA_CMD_READ_DMA 0xc8
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
#define ATA_CMD_WRITE_DMA 0xcaA
#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_CACHE_FLUSH 0xe7
#define ATA_CMD_CACHE_FLUSH_EXT 0xea
#define ATA_CMD_PACKET 0xa0
#define ATA_CMD_IDENTIFY_PACKET 0xa1
#define ATA_CMD_IDENTIFY 0xec

#define ATAPI_CMD_READ 0xa8
#define ATAPI_CMD_EJECT 0x1b

#define ATA_MASTER 0x00
#define ATA_SLAVE 0x01

#define IDE_ATA 0x00
#define IDE_ATAPI 0x01

// ATA-ATAPI Task-File:
#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_FEATURES 0x01
#define ATA_REG_SECCOUNT0 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07
#define ATA_REG_SECCOUNT1 0x08
#define ATA_REG_LBA3 0x09
#define ATA_REG_LBA4 0x0a
#define ATA_REG_LBA5 0x0b
#define ATA_REG_CONTROL 0x0c
#define ATA_REG_ALTSTATUS 0x0c
#define ATA_REG_DEVADDRESS 0x0d

// Channels:
#define ATA_PRIMARY 0x00
#define ATA_SECONDARY 0x01

// Directions:
#define ATA_READ 0x00
#define ATA_WRITE 0x01
u8 ide_buf[2048] = {0};
struct channel {
    u16 base;  // I/O Base.
    u16 ctrl;  // Control Base
    u16 bmide; // Bus Master IDE
    u8 nIEN;   // nIEN (No Interrupt);
} channels[2];

static u8 ide_irq_invoked = 0;
static u8 atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct ide_device {
    u8 reserved; // 0 (Empty) or 1 (This Drive really exists).
    u8 channel;  // 0 (Primary Channel) or 1 (Secondary Channel).
    u8 drive;    // 0 (Master Drive) or 1 (Slave Drive).
    u16 type;    // 0: ATA, 1:ATAPI.
    struct identify_device_data ident_device_data;
} ide_devices[4];

void ide_write(u8 channel, u8 reg, u8 data) {
    if (reg < 0x08)
        outb(data, channels[channel].base + reg - 0x00);
    else if (reg < 0x0c)
        outb(data, channels[channel].base + reg - 0x06);
    else if (reg < 0x0e)
        outb(data, channels[channel].ctrl + reg - 0x0a);
    else if (reg < 0x16)
        outb(data, channels[channel].bmide + reg - 0x0e);
}

u8 ide_read(u8 channel, u8 reg) {
    u8 result = '\0';
    if (reg < 0x08)
        result = inb(channels[channel].base + reg - 0x00);
    else if (reg < 0x0c)
        result = inb(channels[channel].base + reg - 0x06);
    else if (reg < 0x0e)
        result = inb(channels[channel].ctrl + reg - 0x0a);
    else
        result = inb(channels[channel].bmide + reg - 0x0e);
    return result;
}

void ide_read_buffer(u8 channel, u8 reg, void *buffer, u32 quads) {
    if (reg < 0x08)
        insl(channels[channel].base + reg - 0x00, buffer, quads);
    else if (reg < 0x0c)
        insl(channels[channel].base + reg - 0x06, buffer, quads);
    else if (reg < 0x0e)
        insl(channels[channel].ctrl + reg - 0x0a, buffer, quads);
    else if (reg < 0x16)
        insl(channels[channel].bmide + reg - 0x0e, buffer, quads);
}

void ide_init(struct pci_device *ide_device) {

    /* int j, k, count = 0; */

    // 1- Detect I/O Ports which interface IDE Controller:
    channels[ATA_PRIMARY].base = 0x1f0;
    channels[ATA_PRIMARY].ctrl = 0x3f6;
    channels[ATA_SECONDARY].base = 0x170;
    channels[ATA_SECONDARY].ctrl = 0x376;
    channels[ATA_PRIMARY].bmide = 0;
    channels[ATA_SECONDARY].bmide = 0;

    // 2- Disable IRQs:
    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    int count = 0;
    // 3- Detect ATA-ATAPI Devices:
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
            u8 err = 0, type = IDE_ATA, status;
            ide_devices[count].reserved = 0; // Assuming that no drive here.

            // (I) Select Drive:
            ide_write(i, ATA_REG_HDDEVSEL, 0xa0 | (j << 4)); // Select Drive.
            // (II) Send ATA Identify Command:
            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

            // (III) Polling:
            if (!(ide_read(i, ATA_REG_STATUS))) // If Status = 0, No Device.
                continue;

            for (;;) {
                status = ide_read(i, ATA_REG_STATUS);
                if ((status & ATA_SR_ERR)) {
                    err = 1;
                    break;
                } // If Err, Device is not ATA.
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
                    break; // Everything is right.
            }

            // (IV) Probe for ATAPI Devices:

            if (err) {
                u8 cl = ide_read(i, ATA_REG_LBA1);
                u8 ch = ide_read(i, ATA_REG_LBA2);

                if (cl == 0x14 && ch == 0xeb)
                    type = IDE_ATAPI;
                else if (cl == 0x69 && ch == 0x96)
                    type = IDE_ATAPI;
                else
                    continue; // Unknown Type (And always not be a device).

                ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
            }

            // (V) Read Identification Space of the Device:
            // (VI) Read Device Parameters:
            ide_devices[count].reserved = 1;
            ide_devices[count].type = type;
            ide_devices[count].channel = i;
            ide_devices[count].drive = j;

            ide_read_buffer(i, ATA_REG_DATA, ide_buf, 128);
            memcpy(&ide_devices[count].ident_device_data, ide_buf,
                   sizeof(struct identify_device_data));
            count++;
        }

    // 4- Print Summary:
    u8 buf[41] = {0};
    for (int i = 0; i < 4; i++)
        if (ide_devices[i].reserved == 1) {
            struct identify_device_data *ident_data =
                &ide_devices[i].ident_device_data;
            printk("size %d\n", ident_data->CurrentSectorCapacity);

            u8 *model = ident_data->ModelNumber;
            for (int i = 0; i < 40; i += 2) {
                buf[i] = model[i + 1];
                buf[i + 1] = model[i];
            }

            printk(" Found %s Drive %dMB - %s\n",
                   (const char *[]){"ATA",
                                    "ATAPI"}[ide_devices[i].type], /* Type */
                   ident_data->CurrentSectorCapacity * 512 / 1024 /
                       1024, /* Size */
                   buf);

            printk(" LogicalSectorLongerThan256Words %d\n",
                   ident_data->PhysicalLogicalSectorSize
                       .LogicalSectorLongerThan256Words);
            printk(" MultipleLogicalSectorsPerPhysicalSector %d\n",
                   ident_data->PhysicalLogicalSectorSize
                       .MultipleLogicalSectorsPerPhysicalSector);
            printk(" logical sector per physical sector %d\n",
                   ident_data->PhysicalLogicalSectorSize
                       .LogicalSectorsPerPhysicalSector);
        }
    // 5- Enable IRQs:
    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 0);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 0);
}

u8 ide_polling(u8 channel, bool advanced_check) {

    // (I) Delay 400 nanosecond for BSY to be set:
    // -------------------------------------------------
    ide_read(channel,
             ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
    ide_read(channel,
             ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
    ide_read(channel,
             ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
    ide_read(channel,
             ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

    // (II) Wait for BSY to be cleared:
    // -------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait for BSY to be zero.

    if (advanced_check) {

        u8 state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.

        // (III) Check For Errors:
        // -------------------------------------------------
        if (state & ATA_SR_ERR)
            return 2; // Error.

        // (IV) Check If Device fault:
        // -------------------------------------------------
        if (state & ATA_SR_DF)
            return 1; // Device Fault.

        // (V) Check DRQ:
        // -------------------------------------------------
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        if (!(state & ATA_SR_DRQ))
            return 3; // DRQ should be set
    }

    return 0; // No Error.
}

enum ata_access_mode { CHS_MODE, LBA28_MODE, LBA48_MODE };

unsigned char ide_ata_access(u8 direction, u8 drive, u32 lba, u8 numsects,
                             void *addr) {
    enum ata_access_mode access_mode;
    bool dma;
    u8 cmd;
    u8 lba_io[6];
    u8 channel = ide_devices[drive].channel; // Read the Channel.
    u8 slavebit = ide_devices[drive].drive;  // Read the Drive [Master/Slave]
    u32 bus = channels[channel]
                  .base; // The Bus Base, like [0x1F0] which is also data port.
    u32 words = 256; // Approximatly all ATA-Drives has sector-size of 512-byte.
    u16 cyl, i;
    u8 head, sect, err;
    ide_write(channel, ATA_REG_CONTROL,
              channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);

    // (I) Select one from LBA28, LBA48 or CHS;
    if (ide_devices[drive]
            .ident_device_data.CommandSetSupport
            .BigLba) { // Sure Drive should support LBA in this case, or
                       // you are giving a wrong LBA.
        access_mode = LBA48_MODE;
        lba_io[0] = (lba & 0x000000ff) >> 0;
        lba_io[1] = (lba & 0x0000ff00) >> 8;
        lba_io[2] = (lba & 0x00ff0000) >> 16;
        lba_io[3] = (lba & 0xff000000) >> 24;
        lba_io[4] = 0; // We said that we lba is integer, so 32-bit are enough
                       // to access 2TB.
        lba_io[5] = 0; // We said that we lba is integer, so 32-bit are enough
                       // to access 2TB.
        head = 0;      // Lower 4-bits of HDDEVSEL are not used here.
    } else if (ide_devices[drive]
                   .ident_device_data.Capabilities
                   .LbaSupported) { // Drive supports LBA?
        access_mode = LBA28_MODE;
        lba_io[0] = (lba & 0x00000ff) >> 0;
        lba_io[1] = (lba & 0x000ff00) >> 8;
        lba_io[2] = (lba & 0x0ff0000) >> 16;
        lba_io[3] = 0; // These Registers are not used here.
        lba_io[4] = 0; // These Registers are not used here.
        lba_io[5] = 0; // These Registers are not used here.
        head = (lba & 0xF000000) >> 24;
    } else {
        access_mode = CHS_MODE;
        sect = (lba % 63) + 1;
        cyl = (lba + 1 - sect) / (16 * 63);
        lba_io[0] = sect;
        lba_io[1] = (cyl >> 0) & 0xff;
        lba_io[2] = (cyl >> 8) & 0xff;
        lba_io[3] = 0;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba + 1 - sect) % (16 * 63) /
               (63); // Head number is written to HDDEVSEL lower 4-bits.
    }

    // (II) See if Drive Supports DMA or not;
    dma = 0; // Supports or doesn't, we don't support !!!

    // (III) Wait if the drive is busy;
    while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
        ; // Wait if Busy.

    // (IV) Select Drive from the controller;
    if (access_mode == CHS_MODE)
        ide_write(channel, ATA_REG_HDDEVSEL,
                  0xa0 | (slavebit << 4) | head); // Select Drive CHS.
    else
        ide_write(channel, ATA_REG_HDDEVSEL,
                  0xe0 | (slavebit << 4) | head); // Select Drive LBA.

    // (V) Write Parameters;
    if (access_mode == LBA48_MODE) {
        ide_write(channel, ATA_REG_SECCOUNT1, 0);
        ide_write(channel, ATA_REG_LBA3, lba_io[3]);
        ide_write(channel, ATA_REG_LBA4, lba_io[4]);
        ide_write(channel, ATA_REG_LBA5, lba_io[5]);
    }
    ide_write(channel, ATA_REG_SECCOUNT0, numsects);
    ide_write(channel, ATA_REG_LBA0, lba_io[0]);
    ide_write(channel, ATA_REG_LBA1, lba_io[1]);
    ide_write(channel, ATA_REG_LBA2, lba_io[2]);

    // (VI) Select the command and send it;
    // Routine that is followed:
    // If ( DMA & LBA48)   DO_DMA_EXT;
    // If ( DMA & LBA28)   DO_DMA_LBA;
    // If ( DMA & LBA28)   DO_DMA_CHS;
    // If (!DMA & LBA48)   DO_PIO_EXT;
    // If (!DMA & LBA28)   DO_PIO_LBA;
    // If (!DMA & !LBA#)   DO_PIO_CHS;

    if (access_mode == CHS_MODE && dma == 0 && direction == 0)
        cmd = ATA_CMD_READ_PIO;
    if (access_mode == LBA28_MODE && dma == 0 && direction == 0)
        cmd = ATA_CMD_READ_PIO;
    if (access_mode == LBA48_MODE && dma == 0 && direction == 0)
        cmd = ATA_CMD_READ_PIO_EXT;
    if (access_mode == CHS_MODE && dma == 1 && direction == 0)
        cmd = ATA_CMD_READ_DMA;
    if (access_mode == LBA28_MODE && dma == 1 && direction == 0)
        cmd = ATA_CMD_READ_DMA;
    if (access_mode == LBA48_MODE && dma == 1 && direction == 0)
        cmd = ATA_CMD_READ_DMA_EXT;

    if (access_mode == CHS_MODE && dma == 0 && direction == 1)
        cmd = ATA_CMD_WRITE_PIO;
    if (access_mode == LBA28_MODE && dma == 0 && direction == 1)
        cmd = ATA_CMD_WRITE_PIO;
    if (access_mode == LBA48_MODE && dma == 0 && direction == 1)
        cmd = ATA_CMD_WRITE_PIO_EXT;
    if (access_mode == CHS_MODE && dma == 1 && direction == 1)
        cmd = ATA_CMD_WRITE_DMA;
    if (access_mode == LBA28_MODE && dma == 1 && direction == 1)
        cmd = ATA_CMD_WRITE_DMA;
    if (access_mode == LBA48_MODE && dma == 1 && direction == 1)
        cmd = ATA_CMD_WRITE_DMA_EXT;
    ide_write(channel, ATA_REG_COMMAND, cmd); // Send the Command.

    if (dma)
        if (direction == 0)
            ;
        // DMA Read.
        else
            ; // DMA Write.
    else if (direction == 0)
        // PIO Read.
        for (i = 0; i < numsects; i++) {
            if ((err = ide_polling(channel, 1)))
                return err; // Polling, then set error and exit if there is.
            insw(bus, addr, words);
            addr += (words * 2);
        }
    else {
        // PIO Write.
        for (i = 0; i < numsects; i++) {
            ide_polling(channel, 0); // Polling.
            outsw(bus, addr, words);
            addr += (words * 2);
        }
        ide_write(channel, ATA_REG_COMMAND,
                  (char[]){ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH,
                           ATA_CMD_CACHE_FLUSH_EXT}[access_mode]);
        ide_polling(channel, 0); // Polling.
    }

    return 0; // Easy, ... Isn't it?
}

u8 ide_print_error(u32 drive, u8 err) {

    if (err == 0)
        return err;

    printk(" IDE:");
    if (err == 1) {
        printk("- Device Fault\n     ");
        err = 19;
    } else if (err == 2) {
        u8 st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
        if (st & ATA_ER_AMNF) {
            printk("- No Address Mark Found\n     ");
            err = 7;
        }
        if (st & ATA_ER_TK0NF) {
            printk("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_ABRT) {
            printk("- Command Aborted\n     ");
            err = 20;
        }
        if (st & ATA_ER_MCR) {
            printk("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_IDNF) {
            printk("- ID mark not Found\n     ");
            err = 21;
        }
        if (st & ATA_ER_MC) {
            printk("- No Media or Media Error\n     ");
            err = 3;
        }
        if (st & ATA_ER_UNC) {
            printk("- Uncorrectable Data Error\n     ");
            err = 22;
        }
        if (st & ATA_ER_BBK) {
            printk("- Bad Sectors\n     ");
            err = 13;
        }
    } else if (err == 3) {
        printk("- Reads Nothing\n     ");
        err = 23;
    } else if (err == 4) {
        printk("- Write Protected\n     ");
        err = 8;
    }
    printk("- [%s %s] \n",
           (const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel],
           (const char *[]){"Master", "Slave"}[ide_devices[drive].drive]);

    return err;
}

void ide_wait_irq() {
    while (!ide_irq_invoked)
        ;
    ide_irq_invoked = 0;
}

void ide_irq() { ide_irq_invoked = 1; }

unsigned char ide_atapi_read(u8 drive, u32 lba, unsigned char numsects,
                             void *addr) {
    u8 channel = ide_devices[drive].channel;
    u8 slavebit = ide_devices[drive].drive;
    u16 bus = channels[channel].base;
    u32 words = 2048 / 2; // Sector Size in Words, Almost All ATAPI
                          // Drives has a sector size of 2048 bytes.
    u8 err;
    // Enable IRQs:
    ide_write(channel, ATA_REG_CONTROL,
              channels[channel].nIEN = ide_irq_invoked = 0x0);
    // (I): Setup SCSI Packet:
    // ------------------------------------------------------------------
    atapi_packet[0] = ATAPI_CMD_READ;
    atapi_packet[1] = 0x0;
    atapi_packet[2] = (lba >> 24) & 0xFF;
    atapi_packet[3] = (lba >> 16) & 0xFF;
    atapi_packet[4] = (lba >> 8) & 0xFF;
    atapi_packet[5] = (lba >> 0) & 0xFF;
    atapi_packet[6] = 0x0;
    atapi_packet[7] = 0x0;
    atapi_packet[8] = 0x0;
    atapi_packet[9] = numsects;
    atapi_packet[10] = 0x0;
    atapi_packet[11] = 0x0;
    // (II): Select the Drive:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_HDDEVSEL, slavebit << 4);
    // (III): Delay 400 nanosecond for select to complete:
    // ------------------------------------------------------------------
    ide_read(channel,
             ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
    ide_read(channel,
             ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
    ide_read(channel,
             ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
    ide_read(
        channel,
        ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
                            // (IV): Inform the Controller that we use PIO mode:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_FEATURES,
              0); // PIO mode.
                  // (V): Tell the Controller the size of buffer:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_LBA1,
              (words * 2) & 0xFF); // Lower Byte of Sector Size.
    ide_write(channel, ATA_REG_LBA2,
              (words * 2) >> 8); // Upper Byte of Sector Size.
                                 // (VI): Send the Packet Command:
    // ------------------------------------------------------------------
    ide_write(channel, ATA_REG_COMMAND,
              ATA_CMD_PACKET); // Send the Command.
                               // (VII): Waiting for the driver to finish or
                               // invoke an error:
    // ------------------------------------------------------------------
    if ((err = ide_polling(channel, 1)))
        return err; // Polling and return if error.
                    // (VIII): Sending the packet data:
    // ------------------------------------------------------------------
    asm("rep   outsw" ::"c"(6), "d"(bus),
        "S"(atapi_packet)); // Send Packet Data
                            // (IX): Recieving Data:
    // ------------------------------------------------------------------
    for (int i = 0; i < numsects; i++) {
        ide_wait_irq(); // Wait for an IRQ.
        if ((err = ide_polling(channel, 1)))
            return err; // Polling and return if error.

        insw(bus, addr, words);
        addr += (words * 2);
    }
    // (X): Waiting for an IRQ:
    // ------------------------------------------------------------------
    ide_wait_irq();

    // (XI): Waiting for BSY & DRQ to clear:
    // ------------------------------------------------------------------
    while (ide_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ))
        ;

    return 0; // Easy, ... Isn't it?
}

void ide_read_sectors(u8 drive, u8 numsects, u32 lba, void *addr) {

    u8 err = 0;
    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0)
        err = 0x1; // Drive Not Found!

    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba + numsects) >
              ide_devices[drive].ident_device_data.CurrentSectorCapacity) &&
             (ide_devices[drive].type == IDE_ATA))
        err = 0x2; // Seeking to invalid position.

    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        if (ide_devices[drive].type == IDE_ATA)
            err = ide_ata_access(ATA_READ, drive, lba, numsects, addr);
        else if (ide_devices[drive].type == IDE_ATAPI)
            for (size_t i = 0; i < numsects; i++)
                err = ide_atapi_read(drive, lba + i, 1, addr + (i * 2048));
        err = ide_print_error(drive, err);
    }
}
/* package[0] is an entry of array, this entry specifies the Error Code, you can
 */
/* replace that. */
void ide_write_sectors(unsigned char drive, unsigned char numsects,
                       unsigned int lba, void *addr) {

    u8 err = 0;
    // 1: Check if the drive presents:
    // ==================================
    if (drive > 3 || ide_devices[drive].reserved == 0)
        err = 0x1; // Drive Not Found!
    // 2: Check if inputs are valid:
    // ==================================
    else if (((lba + numsects) >
              ide_devices[drive].ident_device_data.CurrentSectorCapacity) &&
             (ide_devices[drive].type == IDE_ATA))
        err = 0x2; // Seeking to invalid position.
    // 3: Read in PIO Mode through Polling & IRQs:
    // ============================================
    else {
        if (ide_devices[drive].type == IDE_ATA)
            err = ide_ata_access(ATA_WRITE, drive, lba, numsects, addr);
        else if (ide_devices[drive].type == IDE_ATAPI)
            err = 4; // Write-Protected.
        err = ide_print_error(drive, err);
    }
}
extern void int2f(void);
void ata_init() {
    /* irq_set_handler(0x2f, int2f); */
    struct pci_device *pci_device;
    list_for_each_entry(pci_device, &pci_devices, list) {
        if (pci_device->config.class_code == 1 &&
            pci_device->config.sub_class == 1) {
            break;
        }
    }

    printk("bus %d device %d function %d\n", pci_device->bus,
           pci_device->device, pci_device->function);
    printk("vendor id = %#x, device id = %#x\n", pci_device->config.vendor_id,
           pci_device->config.device_id);
    printk("class code = %d, sub class = %d\n", pci_device->config.class_code,
           pci_device->config.sub_class);
    printk("prog if %#x\n", pci_device->config.prog_if);
    u32 *bar = &pci_device->config.bar0;
    for (int i = 0; i < 6; i++) {
        printk("bar %d = %#x\n", i, *bar++);
    }

    ide_init(pci_device);

    char buf[513] = {0};
    ide_read_sectors(0, 1, 0, buf);
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 10; ++j) {
            printk("%x ", (u8)buf[i * 10 + j]);
        }
        printk("\n");
    }
}

void do_ata(struct pt_regs *regs) {}
