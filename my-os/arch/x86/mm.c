#include <asm/apic.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/sections.h>

#include <kernel/mm.h>
#include <kernel/printk.h>

#include <my-os/buddy_alloc.h>
#include <my-os/kernel.h>
#include <my-os/log2.h>
#include <my-os/memblock.h>
#include <my-os/mm_types.h>
#include <my-os/string.h>

struct mm_struct init_mm = {.top_page = early_pml4t};

#define pml4e_offset_k(addr)                                                   \
    init_mm.top_page + pml4e_index((unsigned long)(addr))

static phys_addr_t _brk_end = (phys_addr_t)_brk_base;
static unsigned long pgt_buf_start;
static unsigned long pgt_buf_end;
static unsigned long pgt_buf_top;

#define __VMEMMAP_BASE_L4 0xffffea0000000000UL
unsigned long vmemmap_base = __VMEMMAP_BASE_L4;

void *extend_brk(size_t size, size_t align) {
    size_t mask = align - 1;
    void *ret;

    _brk_end = (_brk_end + mask) & ~mask;

    ret = (void *)_brk_end;
    _brk_end += size;

    bzero(ret, size);
    return ret;
}

void *alloc_low_pages(size_t num) {

    unsigned long pfn;

    if ((pgt_buf_end + num) > pgt_buf_top) {
        phys_addr_t brk = __pa(extend_brk(PTE_SIZE * num, PTE_SIZE));
        pfn = brk >> PTE_SHIFT;
    } else {
        pfn = pgt_buf_end;
        pgt_buf_end += num;
        printk("BRK [%p, %p] PGTABLE\n", pfn << PTE_SHIFT,
               (pgt_buf_end << PTE_SHIFT) - 1);
    }

    /* for (size_t i = 0; i < num; i++) { */
    /*     void *addr = __va((pfn + i) << PTE_SHIFT); */
    /*     bzero(addr, PTE_SIZE); */
    /* } */
    return __va(pfn << PTE_SHIFT);
}

#define INIT_PGE_PAGE_COUNT 6
#define INIT_PGT_BUF_SIZE (INIT_PGE_PAGE_COUNT * PTE_SIZE)

void early_alloc_pgt_buf(void) {
    unsigned long tables = INIT_PGT_BUF_SIZE;
    phys_addr_t base;

    base = __pa(extend_brk(tables, PTE_SIZE));

    pgt_buf_start = base >> PTE_SHIFT;
    pgt_buf_end = pgt_buf_start;
    pgt_buf_top = pgt_buf_start + (tables >> PTE_SHIFT);
}

static void set_pte(pte_t *pte, pte_t value) { *pte = value; }

static void set_pte_init(pte_t *pde, phys_addr_t addr) {
    set_pte(pde, (addr | _PAGE_RW | _PAGE_PRESENT));
}

static phys_addr_t phys_pt_init(pte_t *pte_page, phys_addr_t paddr_start,
                                phys_addr_t paddr_end) {
    unsigned long paddr_last = paddr_end;
    unsigned long paddr_next = paddr_start;
    int i = pte_index((unsigned long)__va(paddr_start));

    pte_t *pte;

    for (; i < PTRS_PER_PDE; i++) {
        pte = pte_page + i;

        if (paddr_next >= paddr_end) {
            break;
        }

        if (*pte) {
            continue;
        }

        set_pte_init(pte, paddr_next);
        paddr_last = (paddr_next & PTE_MASK) + PTE_SIZE;

        paddr_next = paddr_last;
    }
    return paddr_last;
}

static void set_pde(pde_t *pde, pde_t value) { *pde = value; }

static void set_pde_init(pde_t *pde, pte_t *pte) {
    set_pde(pde, (__pa(pte) | _PAGE_RW | _PAGE_PRESENT));
}

static phys_addr_t phys_pdt_init(pde_t *pde_page, phys_addr_t paddr_start,
                                 phys_addr_t paddr_end,
                                 unsigned long page_size_mask) {
    unsigned long paddr_last = paddr_end;
    unsigned long paddr_next = paddr_start;
    unsigned long paddr = paddr_start;
    int i = pde_index((unsigned long)__va(paddr_start));

    pde_t *pde;

    for (; i < PTRS_PER_PDE; i++, paddr = paddr_next) {
        pde = pde_page + i;
        pte_t *pte;

        if (paddr_next >= paddr_end) {
            break;
        }

        paddr_next = (paddr_next & PDE_MASK) + PDE_SIZE;

        if (*pde) {
            if (!(*pde & _PAGE_PSE)) {
                pte = (pte_t *)pde_page_vaddr(*pde);
                phys_pt_init(pte, paddr_start, paddr_end);
                continue;
            }

            if (page_size_mask & (1 << PG_LEVEL_2M)) {
                paddr_last = paddr_next;
                continue;
            }
        }

        if (page_size_mask & (1 << PG_LEVEL_2M)) {
            pte_t pte = ((paddr & PDE_MASK) | _PAGE_PSE | _PAGE_KERNEL);
            set_pde(pde, pte);
            paddr_last = paddr_next;
        } else {
            pte = alloc_low_pages(1);
            paddr_last = phys_pt_init(pte, paddr_start, paddr_end);
            set_pde_init(pde, pte);
        }
    }
    return paddr_last;
}

static void set_pdpte(pdpte_t *pdpte, pdpte_t value) { *pdpte = value; }

static void set_pdpte_init(pdpte_t *pdpte, pde_t *pde) {
    set_pdpte(pdpte, (__pa(pde) | _PAGE_RW | _PAGE_PRESENT));
}

static phys_addr_t phys_pdpt_init(pdpte_t *pdpte_page, phys_addr_t paddr_start,
                                  phys_addr_t paddr_end,
                                  unsigned long page_size_mask) {
    unsigned long paddr_last = paddr_end;
    unsigned long paddr_next = paddr_start;
    unsigned long paddr = paddr_start;
    int i = pdpte_index((unsigned long)__va(paddr_start));
    pdpte_t *pdpte;

    for (; i < PTRS_PER_PDPTE; i++, paddr = paddr_next) {
        pdpte = pdpte_page + i;
        pde_t *pde = NULL;

        if (paddr_next >= paddr_end) {
            break;
        }

        paddr_next = (paddr_next & PDPTE_MASK) + PDPTE_SIZE;

        if (*pdpte) {
            if (!(*pdpte & _PAGE_PSE)) {
                pde = (pde_t *)pdpte_page_vaddr(*pdpte);
                phys_pdt_init(pde, paddr_start, paddr_end, page_size_mask);
                continue;
            }

            if (page_size_mask & (1 << PG_LEVEL_1G)) {
                paddr_last = paddr_next;
                continue;
            }
        }

        if (page_size_mask & (1 << PG_LEVEL_1G)) {
            pde_t pde = ((paddr & PDPTE_MASK) | _PAGE_PSE | _PAGE_KERNEL);
            set_pdpte(pdpte, pde);
            paddr_last = paddr_next;
        } else {
            pde = alloc_low_pages(1);
            paddr_last =
                phys_pdt_init(pde, paddr_start, paddr_end, page_size_mask);
            set_pdpte_init(pdpte, pde);
        }
    }

    return paddr_last;
}

static void set_pml4e(pml4e_t *pml4e, pml4e_t value) { *pml4e = value; }

static void set_pml4e_init(pml4e_t *pml4e, pdpte_t *pdpte) {
    set_pml4e(pml4e, (__pa(pdpte) | _PAGE_RW | _PAGE_PRESENT));
}

phys_addr_t kernel_physical_mapping_init(phys_addr_t paddr_start,
                                         phys_addr_t paddr_end,
                                         unsigned long page_size_mask) {

    unsigned long paddr_last;
    unsigned long paddr_next = paddr_start;
    int i = pml4e_index((unsigned long)__va(paddr_start));
    pml4e_t *pml4e;

    for (; paddr_next < paddr_end; i++) {
        pml4e = init_mm.top_page + i;
        pdpte_t *pdpte;
        paddr_next = (paddr_next & PML4E_MASK) + PML4E_SIZE;

        if (*pml4e) {
            pdpte = (pdpte_t *)pml4e_page_vaddr(*pml4e);
            paddr_last =
                phys_pdpt_init(pdpte, paddr_start, paddr_end, page_size_mask);
            continue;
        }
        pdpte = alloc_low_pages(1);
        paddr_last =
            phys_pdpt_init(pdpte, paddr_start, paddr_end, page_size_mask);

        set_pml4e_init(pml4e, pdpte);

        pml4e++;
    }

    return paddr_last;
}

#define PFN_DOWN(x) ((x) >> PTE_SHIFT)

struct map_range {
    unsigned long start;
    unsigned long end;
    unsigned page_size_mask;
};

#define NR_RANGE_MR 5

static int page_size_mask;

static int save_mr(struct map_range *mr, int nr_range, unsigned long start_pfn,
                   unsigned long end_pfn, unsigned long page_size_mask) {
    if (start_pfn < end_pfn) {
        if (nr_range >= NR_RANGE_MR)
            // TODO: EXIT panic
            printk("run out of range for init_memory_mapping\n");
        mr[nr_range].start = start_pfn << PTE_SHIFT;
        mr[nr_range].end = end_pfn << PTE_SHIFT;
        mr[nr_range].page_size_mask = page_size_mask;
        nr_range++;
    }

    return nr_range;
}

static const char *page_size_string(struct map_range *mr) {
    static const char str_1g[] = "1G";
    static const char str_2m[] = "2M";
    static const char str_4k[] = "4k";

    if (mr->page_size_mask & (1 << PG_LEVEL_1G))
        return str_1g;

    if (mr->page_size_mask & (1 << PG_LEVEL_2M))
        return str_2m;

    return str_4k;
}

static int split_mem_range(struct map_range *mr, int nr_range,
                           phys_addr_t start, phys_addr_t end) {
    unsigned long start_pfn, end_pfn, limit_pfn;
    size_t pfn;
    pfn = start_pfn = PFN_DOWN(start);
    limit_pfn = PFN_DOWN(end);
    end_pfn = round_up(pfn, PFN_DOWN(PDE_SIZE));

    if (end_pfn > limit_pfn) {
        end_pfn = limit_pfn;
    }
    if (start_pfn < end_pfn) {
        nr_range = save_mr(mr, nr_range, start_pfn, end_pfn, 0);
        pfn = end_pfn;
    }

    /* big page (2M) range */
    start_pfn = round_up(pfn, PFN_DOWN(PDE_SIZE));
    end_pfn = round_up(pfn, PFN_DOWN(PDPTE_SIZE));
    if (end_pfn > round_down(limit_pfn, PFN_DOWN(PDE_SIZE)))
        end_pfn = round_down(limit_pfn, PFN_DOWN(PDE_SIZE));

    if (start_pfn < end_pfn) {
        nr_range = save_mr(mr, nr_range, start_pfn, end_pfn,
                           page_size_mask & (1 << PG_LEVEL_2M));
        pfn = end_pfn;
    }

    start_pfn = round_up(pfn, PFN_DOWN(PDPTE_SIZE));
    end_pfn = round_down(limit_pfn, PFN_DOWN(PDPTE_SIZE));
    if (start_pfn < end_pfn) {
        nr_range =
            save_mr(mr, nr_range, start_pfn, end_pfn,
                    page_size_mask & ((1 << PG_LEVEL_2M) | (1 << PG_LEVEL_1G)));
        pfn = end_pfn;
    }

    /* tail is not big page (1G) alignment */
    start_pfn = round_up(pfn, PFN_DOWN(PDE_SIZE));
    end_pfn = round_down(limit_pfn, PFN_DOWN(PDE_SIZE));
    if (start_pfn < end_pfn) {
        nr_range = save_mr(mr, nr_range, start_pfn, end_pfn,
                           page_size_mask & (1 << PG_LEVEL_2M));
        pfn = end_pfn;
    }

    /* tail is not big page (2M) alignment */
    start_pfn = pfn;
    end_pfn = limit_pfn;
    nr_range = save_mr(mr, nr_range, start_pfn, end_pfn, 0);

    /* if (!after_bootmem) */
    /* 	adjust_range_page_size_mask(mr, nr_range); */

    int i;
    /* try to merge same page size and continuous */
    for (i = 0; nr_range > 1 && i < nr_range - 1; i++) {
        unsigned long old_start;
        if (mr[i].end != mr[i + 1].start ||
            mr[i].page_size_mask != mr[i + 1].page_size_mask)
            continue;
        /* move it */
        old_start = mr[i].start;
        memmove(&mr[i], &mr[i + 1],
                (nr_range - 1 - i) * sizeof(struct map_range));
        mr[i--].start = old_start;
        nr_range--;
    }

    for (i = 0; i < nr_range; i++)
        printk(" [mem %p-%p] page %s\n", mr[i].start, mr[i].end - 1,
               page_size_string(&mr[i]));

    return nr_range;
}

unsigned long init_memory_mapping(unsigned long start, unsigned long end) {
    struct map_range mr[NR_RANGE_MR];
    unsigned long ret = 0;
    int nr_range, i;

    printk("init_memory_mapping: [mem %#x-%#x]\n", start, end - 1);

    memset(mr, 0, sizeof(mr));
    nr_range = split_mem_range(mr, 0, start, end);

    for (i = 0; i < nr_range; i++) {
        mr[i].page_size_mask = 1 << PG_LEVEL_2M;
        printk("map range: start = %#x, end = %#x, mask = %#x\n", mr[i].start,
               mr[i].end, mr[i].page_size_mask);
        ret = kernel_physical_mapping_init(mr[i].start, mr[i].end,
                                           mr[i].page_size_mask);
    }

    /* add_pfn_range_mapped(start >> PAGE_SHIFT, ret >> PAGE_SHIFT); */

    return ret >> PTE_SHIFT;
}

void init_mem_mapping(void) {
    init_memory_mapping(0, 0x100000);

    // fix mem io apic
    init_memory_mapping(IOAPIC_DEFAULT_BASE, IOAPIC_DEFAULT_BASE + PTE_SIZE);
    init_memory_mapping(LAPIC_DEFAULT_BASE, LAPIC_DEFAULT_BASE + PTE_SIZE);

    /* init_memory_mapping(__pa(init_mm.start_code),
     * (phys_addr_t)KERNEL_LMA_END); */

    memblock_mem_mapping();
    write_cr3(__pa(init_mm.top_page));
}

void *vmemmap_alloc_block(size_t size) {
    phys_addr_t addr = _alloc_pages(get_order(size));
    if (!addr) {
        return NULL;
    }
    void *vaddr = __va(addr);
    memset(vaddr, 0, size);
    return vaddr;
}

pml4e_t *vmemmap_pml4d_populate(unsigned long addr) {
    pml4e_t *pml4d = pml4e_offset_k(addr);
    if (!*pml4d) {
        void *p = vmemmap_alloc_block(PAGE_SIZE);
        if (!p) {
            return NULL;
        }
        set_pml4e_init(pml4d, p);
    }
    return pml4d;
}

pdpte_t *vmemmap_pdptd_populate(pml4e_t *pml4e, unsigned long addr) {
    pdpte_t *pdpte = pdpte_offset(pml4e, addr);
    if (!*pdpte) {
        void *p = vmemmap_alloc_block(PAGE_SIZE);
        if (!p) {
            return NULL;
        }
        set_pdpte_init(pdpte, p);
    }
    return pdpte;
}

pde_t *vmemmap_pded_populate(pdpte_t *pdpte, unsigned long addr) {
    pde_t *pde = pde_offset(pdpte, addr);
    if (!*pde) {
        void *p = vmemmap_alloc_block(PAGE_SIZE);
        if (!p) {
            return NULL;
        }
        set_pde_init(pde, p);
    }
    return pde;
}

pte_t *vmemmap_ptd_populate(pde_t *pde, unsigned long addr) {
    pte_t *pte = pte_offset(pde, addr);
    if (!*pte) {
        void *p = vmemmap_alloc_block(PAGE_SIZE);
        if (!p) {
            return NULL;
        }
        set_pte_init(pte, __pa(p));
    }
    return pte;
}

int vmemmap_populate_basepages(unsigned long start, unsigned long end) {
    unsigned long addr = start;
    pml4e_t *pml4d;
    pdpte_t *pdptd;
    pde_t *pded;
    pte_t *ptd;

    for (; addr < end; addr += PAGE_SIZE) {
        /* printk("addr: %p\n", addr); */
        pml4d = vmemmap_pml4d_populate(addr);
        /* printk("pml4e: %p = %#x\n", pml4d, *pml4d); */
        if (!pml4d) {
            return -1;
        }
        pdptd = vmemmap_pdptd_populate(pml4d, addr);
        /* printk("pdpte: %p = %#x\n", pdptd, *pdptd); */
        if (!pdptd) {
            return -1;
        }
        pded = vmemmap_pded_populate(pdptd, addr);
        /* printk("pde: %p = %#x\n", pded, *pded); */
        if (!pded) {
            return -1;
        }
        ptd = vmemmap_ptd_populate(pded, addr);
        /* printk("pte: %p = %#x\n", ptd, *ptd); */
        if (!ptd) {
            return -1;
        }
        /* printk("\n"); */
    }
    return 0;
}

void init_mapping_mempage(phys_addr_t start, phys_addr_t end) {
    struct page *start_page = pfn_to_page(start >> PAGE_SHIFT);
    struct page *end_page = pfn_to_page(end >> PAGE_SHIFT);
    printk("vmemmap %p-%p\n", start_page, end_page);

    vmemmap_populate_basepages((unsigned long)start_page,
                               (unsigned long)end_page);
}

void mem_init() {

    init_mm.start_code = (unsigned long)_text;
    init_mm.end_code = (unsigned long)_etext;
    init_mm.start_data = (unsigned long)_data;
    init_mm.end_data = (unsigned long)_edata;
    init_mm.start_brk = (unsigned long)_brk_base;

    init_mapping_mempage((phys_addr_t)KERNEL_LMA_END, end_pfn << PAGE_SHIFT);
}
