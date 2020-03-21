BUILD_DIR ?= /home/yydcnjjw/workspace/code/project/my-os/.obj

OBJS = my-os

.PHONY: all $(OBJS)

all: $(BUILD_DIR) $(OBJS) 

$(OBJS):
	$(MAKE) -C $@

$(BUILD_DIR):
	mkdir -p $@

my-os.iso: $(OBJS)
	@mkdir -p $(BUILD_DIR)/iso
	@cp -r boot $(BUILD_DIR)/iso
	@cp $(BUILD_DIR)/my-os/my-os.kernel $(BUILD_DIR)/iso/boot
	@grub-mkrescue -o $(BUILD_DIR)/$@ $(BUILD_DIR)/iso 2> /dev/null

bochs: my-os.iso
	bochs -f bochs/myos.bxrc -q

qemu_args=-smp 2 -m 64 -no-reboot -cdrom .obj/my-os.iso -drive format=raw,file=.obj/disk.img

qemu_debug: my-os.iso
	qemu-system-x86_64 $(qemu_args) -nographic -s -S

qemu_gui_debug: my-os.iso
	qemu-system-x86_64 $(qemu_args) -s -S

qemu: my-os.iso
	qemu-system-x86_64 $(qemu_args) -nographic

qemu_gui: my-os.iso
	qemu-system-x86_64 $(qemu_args)
