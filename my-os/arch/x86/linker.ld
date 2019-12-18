ENTRY(start)

KERNEL_LMA_START = 1M;
KERNEL_VMA_START = 0xffffffff80000000;

SECTIONS
{
    . = KERNEL_LMA_START;
    _boot_start = .;
    
    .bootstrap : 
    {
        * (.multiboot)    
        * (.boot_text)
        * (.boot_data)
        . = ALIGN(4K);
    }
    _boot_end = .;
    
    . += KERNEL_VMA_START;
    _start = .;

    .text : AT(ADDR(.text) - KERNEL_VMA_START)
    {
        _text = .;
        * (.text)
        . = ALIGN(4K);
        _etext = .;
    }
    .data : AT(ADDR(.data) - KERNEL_VMA_START)
    {
        _data = .;
        * (.data)
        . = ALIGN(4K);
        _edata = .;
    }

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA_START)
    {
        _ehframe = .;
        * (.eh_frame)
        . = ALIGN(4K);
    }

    .bss : AT(ADDR(.bss) - KERNEL_VMA_START)
    {
        _bss = .;
        * (.bss)
        * (COMMON)
        . = ALIGN(4K);
        _ebss = .;
    }

    _end_kernel = .;

    .brk : AT(ADDR(.brk) - KERNEL_VMA_START)
    {
        _brk_base = .;
        . += 16 * 1024;        
    }

    KERNEL_VMA_END = .;
    KERNEL_LMA_END = (LOADADDR(.brk) + SIZEOF(.brk) + 0xFFF) & 0xfffffffffffff000;
    _end = .;
}
