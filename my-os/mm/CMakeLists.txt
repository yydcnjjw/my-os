include_directories(
  ${my-os-arch-include}
  ${my-os-include}
)
add_library(my-os-mm memblock.c stack_alloc.c)
