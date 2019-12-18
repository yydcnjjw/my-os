add_subdirectory(x86)

file(WRITE null.c "")

add_library(my-os-arch null.c)
target_link_libraries(my-os-arch
  my-os-arch-x86)
