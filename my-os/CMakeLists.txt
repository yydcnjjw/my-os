set(my-os-include ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS
  "-Wall -Wextra -mcmodel=large -m64 -fno-builtin -fno-stack-protector")

add_subdirectory(arch)
add_subdirectory(init)
add_subdirectory(mm)

file(WRITE null.c "")
add_library(my-os STATIC null.c)

target_link_libraries(my-os
  my-os-arch
  my-os-init
  my-os-mm)
