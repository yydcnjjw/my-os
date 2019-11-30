ENTRY(start)

KERNEL_LMA_START = 1M;
KERNEL_VMA_START = 0xffffffff80000000;

SECTIONS
{
    . = KERNEL_LMA_START;
    _start = .;
    
    .boot : ALIGN(4K)
    {
        * (.multiboot)    
        * (.boot_text)
        * (.boot_data)
        * (.boot_bss)
    }

    . += KERNEL_VMA_START;
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
    .bss : AT(ADDR(.bss) - KERNEL_VMA_START)
    {
        _bss = .;
        * (.bss)
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
    KERNEL_LMA_END = (LOADADDR(.bss) + SIZEOF(.bss) + 0xFFF) & 0xfffffffffffff000;
    _end = .;
}
