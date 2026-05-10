#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>
#include "status.h"

/**
 * @file paging.h
 * @brief x86_64 4-Level Paging Manager.
 *
 * This module provides a basic implementation of long mode paging
 * for x86_64 kernels.
 *
 * Supported paging hierarchy:
 *
 * PML4
 *  └── PDPT
 *       └── PD
 *            └── PT
 *                 └── 4KB PAGE
 *
 * Features:
 * - Dynamic page table allocation
 * - Virtual → physical mapping
 * - Address translation
 * - CR3 switching
 * - Page alignment helpers
 * - TLB invalidation support
 *
 * This implementation currently supports:
 * - 4KB pages
 * - 4-level paging only
 *
 * Unsupported:
 * - Huge pages
 * - 5-level paging
 * - PCID
 * - Copy-on-write
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

/* =========================================================
 * PAGING CONSTANTS
 * ========================================================= */

/** Number of entries per paging table. */
#define PAGING_TOTAL_ENTRIES_PER_TABLE 512

/** Size of a single page in bytes. */
#define PAGING_PAGE_SIZE 4096
#define PAGING_PAGE_SIZE_BYTES PAGING_PAGE_SIZE

/** Page present flag. */
#define PAGING_IS_PRESENT 0x1

/** Writable page flag. */
#define PAGING_IS_WRITEABLE 0x2

/** User-accessible page flag. */
#define PAGING_ACCESSIBLE_FROM_ALL 0x4

/** Write-through caching flag. */
#define PAGING_WRITE_THROUGH 0x8

/** Disable cache flag. */
#define PAGING_CACHE_DISABLED 0x10

/* =========================================================
 * PAGING LEVELS
 * ========================================================= */

/**
 * @brief Supported paging root levels.
 */
typedef enum
{
    PAGING_MAP_LEVEL_4 = 4
} paging_map_level_t;

/* =========================================================
 * PAGING ENTRY STRUCTURE
 * ========================================================= */

/**
 * @brief x86_64 paging entry structure.
 *
 * Represents:
 * - PML4 entry
 * - PDPT entry
 * - PD entry
 * - PT entry
 */
struct paging_desc_entry
{
    uint64_t present : 1;
    uint64_t read_write : 1;
    uint64_t user_supervisor : 1;
    uint64_t write_through : 1;
    uint64_t cache_disable : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t pat : 1;
    uint64_t global : 1;
    uint64_t ignored : 3;
    uint64_t address : 40;
    uint64_t available : 11;
    uint64_t no_execute : 1;
} __attribute__((packed));

/* =========================================================
 * PAGING TABLE STRUCTURES
 * ========================================================= */

/**
 * @brief Generic paging table.
 */
struct paging_pml_entries
{
    struct paging_desc_entry entries[PAGING_TOTAL_ENTRIES_PER_TABLE];
} __attribute__((packed));

/**
 * @brief Paging descriptor.
 */
struct paging_desc
{
    struct paging_pml_entries* pml;
    paging_map_level_t level;
} __attribute__((packed));

/* =========================================================
 * PAGING CORE FUNCTIONS
 * ========================================================= */

/**
 * @brief Create a new paging descriptor.
 *
 * @param root_map_level Paging level.
 *
 * @return New paging descriptor or NULL on failure.
 */
struct paging_desc* paging_desc_new(
    paging_map_level_t root_map_level
);

struct paging_desc* paging_desc_new_identity(
    size_t bytes,
    int flags
);

void paging_desc_free(struct paging_desc* desc);

/**
 * @brief Switch to a paging descriptor.
 *
 * Loads the PML4 into CR3.
 *
 * @param desc Paging descriptor.
 */
void paging_switch(struct paging_desc* desc);

/**
 * @brief Map a single page.
 *
 * @param desc Paging descriptor.
 * @param virt Virtual address.
 * @param phys Physical address.
 * @param flags Page flags.
 *
 * @return STATUS_OK on success.
 */
int paging_map(
    struct paging_desc* desc,
    void* virt,
    void* phys,
    int flags
);

/**
 * @brief Map multiple pages.
 *
 * @param desc Paging descriptor.
 * @param virt Starting virtual address.
 * @param phys Starting physical address.
 * @param count Number of pages.
 * @param flags Page flags.
 *
 * @return STATUS_OK on success.
 */
int paging_map_range(
    struct paging_desc* desc,
    void* virt,
    void* phys,
    size_t count,
    int flags
);

/**
 * @brief Map a physical range to a virtual range.
 *
 * @param desc Paging descriptor.
 * @param virt Starting virtual address.
 * @param phys Starting physical address.
 * @param phys_end Ending physical address.
 * @param flags Page flags.
 *
 * @return STATUS_OK on success.
 */
int paging_map_to(
    struct paging_desc* desc,
    void* virt,
    void* phys,
    void* phys_end,
    int flags
);

/* =========================================================
 * ADDRESS TRANSLATION
 * ========================================================= */

/**
 * @brief Translate a virtual address to physical.
 *
 * @param desc Paging descriptor.
 * @param virt Virtual address.
 *
 * @return Physical address or NULL if unmapped.
 */
void* paging_get_physical_address(
    struct paging_desc* desc,
    void* virt
);

/* =========================================================
 * PAGE ALIGNMENT
 * ========================================================= */

/**
 * @brief Align address upward to next page.
 *
 * @param ptr Address to align.
 *
 * @return Page-aligned address.
 */
void* paging_align_address(void* ptr);

/**
 * @brief Align address downward to page boundary.
 *
 * @param addr Address to align.
 *
 * @return Lower aligned page address.
 */
void* paging_align_to_lower_page(void* addr);

/**
 * @brief Check if address is page aligned.
 *
 * @param addr Address to check.
 *
 * @return true if aligned.
 */
bool paging_is_aligned(void* addr);

/* =========================================================
 * CPU / TLB FUNCTIONS
 * ========================================================= */

/**
 * @brief Load paging directory into CR3.
 *
 * @param directory PML4 address.
 */
void paging_load_directory(uint64_t* directory);

/**
 * @brief Invalidate a single TLB entry.
 *
 * @param addr Virtual address.
 */
void paging_invalidate_tlb_entry(void* addr);

/**
 * Map all usable memory regions from the E820 map.
 * 
 * @param desc Paging descriptor.
 */
void paging_map_e820_memory_regions(struct paging_desc* desc);

#endif /* PAGING_H */
