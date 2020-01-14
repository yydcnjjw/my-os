#ifndef X86_ASM_PGTABLE_H
#define X86_ASM_PGTABLE_H

/* Extracts the PFN from a (pte|pmd|pud|pgd)val_t of a 4KB page */

#include <asm/page.h>
#include <asm/page_types.h>

#define PTE_PFN_MASK (PHYSICAL_PTE_MASK)

static inline pml4e_t pml4e_pfn_mask() { return PTE_PFN_MASK; }

static inline pdpte_t pdpte_pfn_mask(pdpte_t pdpte) {
    if (pdpte & _PAGE_PSE) {
        return PHYSICAL_PDPTE_MASK;
    } else {
        return PTE_PFN_MASK;
    }
}

static inline pde_t pde_pfn_mask(pde_t pde) {
    if (pde & _PAGE_PSE) {
        return PHYSICAL_PDE_MASK;
    } else {
        return PTE_PFN_MASK;
    }
}

static inline unsigned long pml4e_page_vaddr(pml4e_t pml4e) {
    return (unsigned long)__va(pml4e & pml4e_pfn_mask());
}

static inline unsigned long pdpte_page_vaddr(pdpte_t pdpte) {
    return (unsigned long)__va(pdpte & pdpte_pfn_mask(pdpte));
}

static inline unsigned long pde_page_vaddr(pde_t pde) {
    return (unsigned long)__va(pde & pde_pfn_mask(pde));
}

static inline pdpte_t *pdpte_offset(pml4e_t *pml4d, unsigned long addr) {
    return (pdpte_t *)pml4e_page_vaddr(*pml4d) + pdpte_index(addr);
}

static inline pde_t *pde_offset(pdpte_t *pdpte, unsigned long addr) {
    return (pde_t *)pdpte_page_vaddr(*pdpte) + pde_index(addr);
}

static inline pte_t *pte_offset(pde_t *pde, unsigned long addr) {
    return (pte_t *)pde_page_vaddr(*pde) + pte_index(addr);
}

#endif /* X86_ASM_PGTABLE_H */
