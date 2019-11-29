#include <asm/sections.h>
#include <my-os/mm_types.h>

void start_kernel(void) {
    
    struct mm_struct init_mm = {.start_code = (unsigned long)_text,
                                .end_code = (unsigned long)_etext,
                                .start_data = (unsigned long)_data,
                                .end_data = (unsigned long)_edata,
                                .start_brk = (unsigned long)_brk_base
    };
    
}
