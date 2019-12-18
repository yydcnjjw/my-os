set(my-os-arch-include ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "")

add_subdirectory(kernel)

include_directories(
  ${my-os-arch-include}
  ${my-os-include})

add_library(my-os-arch-x86 STATIC
  idt.c
  head64.c
  head_64.S)

target_link_libraries(my-os-arch-x86 my-os-arch-x86-kernel)
