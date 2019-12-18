include_directories(
  ${my-os-arch-include}
  ${my-os-include})

add_library(my-os-arch-x86-kernel STATIC
  early_printk.c
  multiboot2.c
  printk.c
  vsprintf.c)
