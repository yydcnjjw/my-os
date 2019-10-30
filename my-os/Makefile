BUILD_DIR ?= /home/yydcnjjw/workspace/code/project/my-os/.obj/my-os

#TODO : for arch
ARCH = x86
ARCHDIR = arch/$(ARCH)

CFLAGS ?= -O2 -g
CPPFLAGS ?=
LDFLAGS ?=

NASM = nasm
NASM_FLAGS ?= -g

CFLAGS := $(CFLAGS) \
-Wall \
-Wextra \
-fno-strict-aliasing \
-fno-common \
-ffreestanding \
-fno-PIE \
-fno-stack-protector \
-mno-sse \
-mno-mmx \
-mno-sse2 \
-mcmodel=kernel \
-m64 \
-mno-red-zone \
-Iinclude \
-I$(ARCHDIR)/include \
# -fno-pie
LDFLAGS := -n -nostdlib # -fno-stack-protector

OBJS = \
$(ARCHDIR)/kernel/early_printk.o \
$(ARCHDIR)/kernel/printk.o \
$(ARCHDIR)/kernel/vsprintf.o \
$(ARCHDIR)/kernel/multiboot2.o \
$(ARCHDIR)/boot.o \
$(ARCHDIR)/idt.o \
$(ARCHDIR)/head64.o \
mm/memblock.o


.PHONY: all

all: $(BUILD_DIR) my-os.kernel

$(BUILD_DIR):
	@mkdir -p $@

my-os.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	ld -o $(BUILD_DIR)/$@ -T $(ARCHDIR)/linker.ld $(LDFLAGS) $(foreach f, $(OBJS), $(BUILD_DIR)/$(f))
	grub-file --is-x86-multiboot2 $(BUILD_DIR)/$@

%.o: %.c
	$(CC) -MD -c -o $(BUILD_DIR)/$@ $(CFLAGS) $<

%.o: %.nasm
	$(NASM) -MD $(BUILD_DIR)/$(@:.o=.d) -f elf64 -o $(BUILD_DIR)/$@ $(NASM_FLAGS) $<


-include $(foreach f, $(OBJS:.o=.d), $(BUILD_DIR)/$(f))
