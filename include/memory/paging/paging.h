#ifndef PAGING_H
#define PAGING_H

#include "status.h"

#include "stdint.h"
#include "stdbool.h"

/** Page entry flag: disable CPU cache for mapped page. */
#define PAGING_CACHE_DISABLED       0b00010000

/** Page entry flag: enable write-through cache policy. */
#define PAGING_WRITE_THROUGH        0b00001000

/** Page entry flag: allow user-space access. */
#define PAGING_ACCESSIBLE_FROM_ALL  0b00000100

/** Page entry flag: allow writes to mapped page. */
#define PAGING_IS_WRITEABLE         0b00000010

/** Page entry flag: mark page as present. */
#define PAGING_IS_PRESENT           0b00000001

/** Number of entries in a single 32-bit paging table/directory. */
#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024

/** Page size in bytes for this paging mode. */
#define PAGING_PAGE_SIZE_BYTES 4096

/**
 * 4GB paging chunk wrapper containing page directory pointer.
 */
struct paging_4gb_chunk {
    /** Page directory base pointer. */
    uint32_t* directory_entry;
};

/**
 * Create a new 4GB identity-mapped paging chunk.
 *
 * @param flags Page flags to apply to generated entries.
 * @return New paging chunk or NULL on allocation/setup failure.
 */
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags);

/**
 * Switch active paging directory.
 *
 * @param directory Page directory to load.
 * @return None.
 */
void paging_switch(struct paging_4gb_chunk* directory);

/**
 * Set a paging entry for a virtual address.
 *
 * @param directory Page directory.
 * @param virtual_address Virtual address to map/update.
 * @param value Raw page entry value.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t paging_set(struct paging_4gb_chunk* directory, void* virtual_address, uint32_t value);

/**
 * Check whether an address is page-size aligned.
 *
 * @param address Address to test.
 * @return true if aligned to PAGING_PAGE_SIZE_BYTES, otherwise false.
 */
bool paging_is_aligned(void* address);

/**
 * Enable CPU paging after tables are configured.
 *
 * @return None.
 */
void enable_paging();

/**
 * Get page directory pointer from a paging chunk wrapper.
 *
 * @param chunk Paging chunk wrapper.
 * @return Page directory pointer for the chunk.
 */
uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk *chunk);

/**
 * Free a 4GB paging chunk and all associated page tables.
 * 
 * @param chunk Paging chunk to free.
 * @return None.
 */
void paging_free_4gb(struct paging_4gb_chunk *chunk);

/**
 * Map a range of physical memory to a virtual address range in the given directory.
 *
 * @param directory Page directory to modify.
 * @param virtual_address Starting virtual address for the mapping.
 * @param physical_address Starting physical address for the mapping.
 * @param total_pages Total number of pages to map.
 * @param flags Page entry flags to apply to mapped pages.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t paging_map_range(struct paging_4gb_chunk* directory, void* virtual_address, void* physical_address, uint32_t total_pages, uint8_t flags);

/**
 * Map a physical memory region to a virtual address range in the given directory.
 *
 * @param directory Page directory to modify.
 * @param virtual_address Starting virtual address for the mapping.
 * @param physical_address Starting physical address for the mapping.
 * @param flags Page entry flags to apply to mapped pages.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t paging_map_to(struct paging_4gb_chunk* directory, void* virtual_address, void* physical_address, void* physical_address_end, uint8_t flags);

/**
 * Paging align address down to nearest page boundary.
 * 
 * @param address Address to align.
 * @return Aligned address.
 */
void* paging_align_to_lower_page(void* address);

/**
 * Paging align address up to nearest page boundary.
 * 
 * @param address Address to align.
 * @return Aligned address.
 */
void* paging_align_address(void* address);

/**
 * Get the raw page entry value for a virtual address in the given directory.
 *
 * @param directory Page directory to query.
 * @param virtual_address Virtual address to get the entry for.
 * @return Raw page entry value, or 0 if not mapped.
 */
uint32_t paging_get(uint32_t* directory, void* virtual_address);

/**
 * Maps a single page for a virtual address to a physical address in the given directory.
 *
 * @param directory Page directory to modify.
 * @param virtual_address Virtual address to map.
 * @param physical_address Physical address to map to.
 * @param flags Page entry flags to apply to the mapped page.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t paging_map(struct paging_4gb_chunk* directory, void* virtual_address, void* physical_address, uint8_t flags);

/**
 * Get the physical address mapped to a virtual address in the given directory.
 *
 * @param directory Page directory to query.
 * @param virtual_address Virtual address to translate.
 * @return Physical address mapped to the virtual address, or NULL if not mapped.
 */
void* paging_get_physical_address(uint32_t* directory, void* virtual_address);

#endif /* PAGING_H */
