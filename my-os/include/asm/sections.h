#ifndef _ASM_SECTIONS_H
#define _ASM_SECTIONS_H

extern char _boot_start[], _boot_end[];
extern char _start[], _end[], _end_kernel[];
extern char _data[], _edata[], _text[], _etext[], _bss[], _ebss[];
extern char _brk_base[];

extern char KERNEL_LMA_END[];

#endif /* _ASM_SECTIONS_H */
