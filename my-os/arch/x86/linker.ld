ENTRY(start)

KERNEL_LMA_START = 1M;
KERNEL_VMA_START = 0xffffffff80000000;

SECTIONS
{
    . = KERNEL_LMA_START;

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

        * (.text)
        . = ALIGN(4K);
    }
    .data : AT(ADDR(.data) - KERNEL_VMA_START)
    {
        * (.data)
        . = ALIGN(4K);        
    }
    .bss : AT(ADDR(.bss) - KERNEL_VMA_START)
    {
        * (.bss)
        . = ALIGN(4K);
    }

    KERNEL_VMA_END = .;
    KERNEL_LMA_END = (LOADADDR(.bss) + SIZEOF(.bss) + 0xFFF) & 0xfffffffffffff000;
}
