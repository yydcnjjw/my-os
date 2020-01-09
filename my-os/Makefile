BUILD_DIR ?= /home/yydcnjjw/workspace/code/project/my-os/.obj/my-os

#TODO : for arch
ARCH = x86
ARCHDIR = arch/$(ARCH)

# CFLAGS ?= -O2 -g
CFLAGS ?= -g
CPPFLAGS ?=
LDFLAGS ?=

NASM = nasm
NASM_FLAGS ?= -g

CFLAGS := $(CFLAGS) \
-I$(ARCHDIR)/include \
-Iinclude \
-Wall \
-Wextra \
-mcmodel=large \
-m64 \
-fno-builtin \
-fno-stack-protector \

# -mno-red-zone \
# -fno-strict-aliasing \
# -fno-common \
# -ffreestanding \
# -fno-PIE \
# -mno-sse \
# -mno-mmx \
# -mno-sse2 \
# -fno-pie

LDFLAGS := -b elf64-x86-64 # -n # -nostdlib

OBJS = \
$(ARCHDIR)/kernel/early_printk.o \
$(ARCHDIR)/kernel/printk.o \
$(ARCHDIR)/kernel/vsprintf.o \
$(ARCHDIR)/kernel/multiboot2.o \
$(ARCHDIR)/idt.o \
$(ARCHDIR)/gdt.o \
$(ARCHDIR)/apic.o \
$(ARCHDIR)/head_64.o \
$(ARCHDIR)/head64.o \
$(ARCHDIR)/boot.o \
$(ARCHDIR)/mm.o \
lib/string.o \
mm/memblock.o \
mm/stack_alloc.o \
mm/buddy_alloc.o \
init/main.o


.PHONY: all

all: $(BUILD_DIR) my-os.kernel

$(BUILD_DIR):
	@mkdir -p $@

my-os.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	ld -o $(BUILD_DIR)/$@ -T $(ARCHDIR)/linker.ld $(LDFLAGS) $(foreach f, $(OBJS), $(BUILD_DIR)/$(f))
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary $(BUILD_DIR)/$@ $(BUILD_DIR)/$@.bin
	grub-file --is-x86-multiboot2 $(BUILD_DIR)/$@

%.o: %.c
	$(CC) -MD -c -o $(BUILD_DIR)/$@ $(CFLAGS) $<

%.o: %.S
	$(CC) -MD -c -o $(BUILD_DIR)/$@ $(CFLAGS) $<

%.o: %.nasm
	$(NASM) -MD $(BUILD_DIR)/$(@:.o=.d) -f elf64 -o $(BUILD_DIR)/$@ $(NASM_FLAGS) $<


-include $(foreach f, $(OBJS:.o=.d), $(BUILD_DIR)/$(f))
