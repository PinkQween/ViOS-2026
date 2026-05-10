#include "memory/paging/paging.h"
#include "config.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"

static struct paging_desc* current_paging_desc = NULL;

/* =========================================================
 * INTERNAL HELPERS
 * ========================================================= */

static bool paging_map_level_is_valid(
    paging_map_level_t level
)
{
    return level == PAGING_MAP_LEVEL_4;
}

static bool paging_null_entry(
    struct paging_desc_entry* entry
)
{
    struct paging_desc_entry null_entry = {0};

    return memcmp(
        entry,
        &null_entry,
        sizeof(struct paging_desc_entry)
    ) == 0;
}

static struct paging_pml_entries* paging_pml_entries_new()
{
    return kzalloc(sizeof(struct paging_pml_entries));
}

static void paging_apply_flags(struct paging_desc_entry* entry, int flags)
{
    entry->present = (flags & PAGING_IS_PRESENT) ? 1 : 0;
    entry->read_write = (flags & PAGING_IS_WRITEABLE) ? 1 : 0;
    entry->user_supervisor = (flags & PAGING_ACCESSIBLE_FROM_ALL) ? 1 : 0;
    entry->write_through = (flags & PAGING_WRITE_THROUGH) ? 1 : 0;
    entry->cache_disable = (flags & PAGING_CACHE_DISABLED) ? 1 : 0;
}

static void paging_promote_entry_flags(struct paging_desc_entry* entry, int flags)
{
    if (flags & PAGING_ACCESSIBLE_FROM_ALL)
    {
        entry->user_supervisor = 1;
    }

    if (flags & PAGING_IS_WRITEABLE)
    {
        entry->read_write = 1;
    }
}

/* =========================================================
 * ALIGNMENT FUNCTIONS
 * ========================================================= */

bool paging_is_aligned(void* addr)
{
    return ((uintptr_t)addr % PAGING_PAGE_SIZE) == 0;
}

void* paging_align_address(void* ptr)
{
    if ((uintptr_t)ptr % PAGING_PAGE_SIZE)
    {
        return (void*)
        (
            (uintptr_t)ptr +
            PAGING_PAGE_SIZE -
            ((uintptr_t)ptr % PAGING_PAGE_SIZE)
        );
    }

    return ptr;
}

void* paging_align_to_lower_page(void* addr)
{
    uintptr_t address = (uintptr_t)addr;

    address -= address % PAGING_PAGE_SIZE;

    return (void*)address;
}

/* =========================================================
 * PAGING DESCRIPTOR
 * ========================================================= */

struct paging_desc* paging_desc_new(
    paging_map_level_t root_map_level
)
{
    if (!paging_map_level_is_valid(root_map_level))
    {
        return NULL;
    }

    struct paging_desc* desc =
        kzalloc(sizeof(struct paging_desc));

    if (!desc)
    {
        return NULL;
    }

    desc->pml = paging_pml_entries_new();
    if (!desc->pml)
    {
        kfree(desc);
        return NULL;
    }

    desc->level = root_map_level;

    return desc;
}

struct paging_desc* paging_desc_new_identity(size_t bytes, int flags)
{
    struct paging_desc* desc = paging_desc_new(PAGING_MAP_LEVEL_4);
    if (!desc)
    {
        return NULL;
    }

    size_t page_count = (bytes + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE;
    int result = paging_map_range(desc, 0, 0, page_count, flags);
    if (result < 0)
    {
        paging_desc_free(desc);
        return NULL;
    }

    return desc;
}

void paging_desc_free(struct paging_desc* desc)
{
    if (!desc)
    {
        return;
    }

    /*
     * Page-table pages are allocated from the simple block heap. The current
     * heap has no ownership walk for nested paging tables yet, so this releases
     * the descriptor root and keeps the API stable for callers.
     */
    if (desc->pml)
    {
        kfree(desc->pml);
    }

    kfree(desc);
}

void paging_switch(struct paging_desc* desc)
{
    current_paging_desc = desc;

    paging_load_directory(
        (uint64_t*)&desc->pml->entries[0]
    );
}

/* =========================================================
 * PAGE MAPPING
 * ========================================================= */

void paging_map_e820_memory_regions(struct paging_desc* desc)
{
    paging_map_to(desc, (void*)0, (void*)0, (void*)0x100000, PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);
    
    size_t total_entries = e820_total_entries();
    
    for (size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry* entry = e820_get_entry(i);

        if (entry->type == 1)
        {
            void* base_address = (void*)entry->base_addr;
            void* end_address = (void*)(entry->base_addr + entry->length);

            if (!paging_is_aligned(base_address))
            {
                base_address = paging_align_address(base_address);
            }

            if (!paging_is_aligned(end_address))
            {
                end_address = paging_align_to_lower_page(end_address);
            }

            paging_map_to(
                desc,
                base_address,
                base_address,
                end_address,
                PAGING_IS_PRESENT | PAGING_IS_WRITEABLE
            );
        }
    }

    paging_map_to(
        desc,
        (void*)VGA_TEXT_MEMORY_ADDRESS,
        (void*)VGA_TEXT_MEMORY_ADDRESS,
        (void*)(VGA_TEXT_MEMORY_ADDRESS + VGA_TEXT_MEMORY_SIZE),
        PAGING_IS_PRESENT | PAGING_IS_WRITEABLE | PAGING_CACHE_DISABLED
    );
}

int paging_map(
    struct paging_desc* desc,
    void* virt,
    void* phys,
    int flags
)
{
    uintptr_t va = (uintptr_t)virt;

    size_t pml4_index = (va >> 39) & 0x1FF;
    size_t pdpt_index = (va >> 30) & 0x1FF;
    size_t pd_index   = (va >> 21) & 0x1FF;
    size_t pt_index   = (va >> 12) & 0x1FF;

    struct paging_desc_entry* pml4_entry =
        &desc->pml->entries[pml4_index];

    /* =====================================================
     * PML4 -> PDPT
     * ===================================================== */

    if (paging_null_entry(pml4_entry))
    {
        void* new_pdpt =
            kzalloc(
                sizeof(struct paging_desc_entry) *
                PAGING_TOTAL_ENTRIES_PER_TABLE
            );

        pml4_entry->address =
            ((uintptr_t)new_pdpt) >> 12;

        pml4_entry->present = 1;
        pml4_entry->read_write = 1;
    }
    paging_promote_entry_flags(pml4_entry, flags);

    struct paging_desc_entry* pdpt_entries =
        (struct paging_desc_entry*)
        (
            ((uintptr_t)pml4_entry->address) << 12
        );

    struct paging_desc_entry* pdpt_entry =
        &pdpt_entries[pdpt_index];

    /* =====================================================
     * PDPT -> PD
     * ===================================================== */

    if (paging_null_entry(pdpt_entry))
    {
        void* new_pd =
            kzalloc(
                sizeof(struct paging_desc_entry) *
                PAGING_TOTAL_ENTRIES_PER_TABLE
            );

        pdpt_entry->address =
            ((uintptr_t)new_pd) >> 12;

        pdpt_entry->present = 1;
        pdpt_entry->read_write = 1;
    }
    paging_promote_entry_flags(pdpt_entry, flags);

    struct paging_desc_entry* pd_entries =
        (struct paging_desc_entry*)
        (
            ((uintptr_t)pdpt_entry->address) << 12
        );

    struct paging_desc_entry* pd_entry =
        &pd_entries[pd_index];

    /* =====================================================
     * PD -> PT
     * ===================================================== */

    if (paging_null_entry(pd_entry))
    {
        void* new_pt =
            kzalloc(
                sizeof(struct paging_desc_entry) *
                PAGING_TOTAL_ENTRIES_PER_TABLE
            );

        pd_entry->address =
            ((uintptr_t)new_pt) >> 12;

        pd_entry->present = 1;
        pd_entry->read_write = 1;
    }
    paging_promote_entry_flags(pd_entry, flags);

    struct paging_desc_entry* pt_entries =
        (struct paging_desc_entry*)
        (
            ((uintptr_t)pd_entry->address) << 12
        );

    struct paging_desc_entry* pt_entry =
        &pt_entries[pt_index];

    /* =====================================================
     * FINAL PAGE
     * ===================================================== */

    if (!paging_null_entry(pt_entry))
    {
        paging_invalidate_tlb_entry(virt);
    }

    pt_entry->address =
        ((uintptr_t)phys) >> 12;

    paging_apply_flags(pt_entry, flags);

    return STATUS_OK;
}

int paging_map_range(
    struct paging_desc* desc,
    void* virt,
    void* phys,
    size_t count,
    int flags
)
{
    int result = STATUS_OK;

    for (size_t i = 0; i < count; i++)
    {
        result = paging_map(
            desc,
            virt,
            phys,
            flags
        );

        if (result < 0)
        {
            break;
        }

        virt += PAGING_PAGE_SIZE;
        phys += PAGING_PAGE_SIZE;
    }

    return result;
}

int paging_map_to(
    struct paging_desc* desc,
    void* virt,
    void* phys,
    void* phys_end,
    int flags
)
{
    if (!paging_is_aligned(virt) ||
        !paging_is_aligned(phys) ||
        !paging_is_aligned(phys_end))
    {
        return -1;
    }

    if ((uintptr_t)phys_end < (uintptr_t)phys)
    {
        return -1;
    }

    uint64_t total_bytes =
        (uintptr_t)phys_end -
        (uintptr_t)phys;

    size_t total_pages =
        total_bytes / PAGING_PAGE_SIZE;

    return paging_map_range(
        desc,
        virt,
        phys,
        total_pages,
        flags
    );
}

/* =========================================================
 * ADDRESS TRANSLATION
 * ========================================================= */

void* paging_get_physical_address(
    struct paging_desc* desc,
    void* virt
)
{
    uintptr_t va = (uintptr_t)virt;

    size_t pml4_index = (va >> 39) & 0x1FF;
    size_t pdpt_index = (va >> 30) & 0x1FF;
    size_t pd_index   = (va >> 21) & 0x1FF;
    size_t pt_index   = (va >> 12) & 0x1FF;

    struct paging_desc_entry* pml4_entry =
        &desc->pml->entries[pml4_index];

    if (!pml4_entry->present)
    {
        return NULL;
    }

    struct paging_desc_entry* pdpt =
        (struct paging_desc_entry*)
        (
            ((uintptr_t)pml4_entry->address) << 12
        );

    struct paging_desc_entry* pdpt_entry =
        &pdpt[pdpt_index];

    if (!pdpt_entry->present)
    {
        return NULL;
    }

    struct paging_desc_entry* pd =
        (struct paging_desc_entry*)
        (
            ((uintptr_t)pdpt_entry->address) << 12
        );

    struct paging_desc_entry* pd_entry =
        &pd[pd_index];

    if (!pd_entry->present)
    {
        return NULL;
    }

    struct paging_desc_entry* pt =
        (struct paging_desc_entry*)
        (
            ((uintptr_t)pd_entry->address) << 12
        );

    struct paging_desc_entry* pt_entry =
        &pt[pt_index];

    if (!pt_entry->present)
    {
        return NULL;
    }

    uintptr_t phys =
        (((uintptr_t)pt_entry->address) << 12) |
        (va & 0xFFF);

    return (void*)phys;
}
