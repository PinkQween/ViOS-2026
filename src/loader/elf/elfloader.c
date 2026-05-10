#include "loader/elf/elfloader.h"
#include "loader/elf/elf.h"
#include "fs/file.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "config.h"

#include "stdint.h"

/**
 * @file elfloader.c
 * @brief Comprehensive ELF32 Loader with Dynamic Linking support.
 */

#define ELF_MAX_FILE_SIZE (1024 * 1024)
#define ELF_SEGMENT_BSS_SLACK 4096

static bool elf_range_in_file(uint32_t offset, uint32_t size, uint32_t file_size)
{
    if (offset > file_size) {
        return false;
    }

    return size <= file_size - offset;
}

static void* elf_file_ptr(struct elf_file* file, uint32_t offset, uint32_t size)
{
    if (!file || !elf_range_in_file(offset, size, file->in_memory_size)) {
        return NULL;
    }

    return (void*)((uintptr_t)file->elf_memory + offset);
}

static status_t elf_validate32(struct elf32_header* header, uint32_t file_size)
{
    if (!header || file_size < sizeof(struct elf32_header)) {
        return STATUS_ERR(EINVAL);
    }

    if (!elf_is_valid(header) ||
        header->e_ident[EI_CLASS] != ELFCLASS32 ||
        header->e_ident[EI_DATA] != ELFDATA2LSB ||
        header->e_ident[EI_VERSION] != EV_CURRENT ||
        header->e_type != ET_EXEC ||
        header->e_machine != EM_386 ||
        header->e_version != EV_CURRENT ||
        header->e_ehsize != sizeof(struct elf32_header) ||
        header->e_phentsize != sizeof(struct elf32_phdr)) {
        return STATUS_ERR(EINVAL);
    }

    if (header->e_phnum == 0 ||
        header->e_phnum > ELF_MAX_SEGMENTS ||
        !elf_range_in_file(header->e_phoff, header->e_phnum * header->e_phentsize, file_size)) {
        return STATUS_ERR(EINVAL);
    }

    if (header->e_shoff != 0) {
        if (header->e_shentsize != sizeof(struct elf32_shdr) ||
            !elf_range_in_file(header->e_shoff, header->e_shnum * header->e_shentsize, file_size)) {
            return STATUS_ERR(EINVAL);
        }
    }

    bool entry_in_load_segment = false;
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr* phdr = (struct elf32_phdr*)((uintptr_t)header + header->e_phoff + i * header->e_phentsize);

        if (phdr->p_filesz > phdr->p_memsz) {
            return STATUS_ERR(EINVAL);
        }

        if (phdr->p_filesz && !elf_range_in_file(phdr->p_offset, phdr->p_filesz, file_size)) {
            return STATUS_ERR(EINVAL);
        }

        if (phdr->p_memsz > phdr->p_filesz + ELF_SEGMENT_BSS_SLACK) {
            return STATUS_ERR(EINVAL);
        }

        if (phdr->p_type == PT_LOAD &&
            phdr->p_vaddr <= 0xFFFFFFFFu - phdr->p_memsz &&
            header->e_entry >= phdr->p_vaddr &&
            header->e_entry < phdr->p_vaddr + phdr->p_memsz) {
            entry_in_load_segment = true;
        }
    }

    return entry_in_load_segment ? STATUS_OK : STATUS_ERR(EINVAL);
}

static void* elf_vaddr_to_file_ptr(struct elf_file* file, uintptr_t vaddr, size_t size)
{
    for (int i = 0; i < file->segment_count; i++) {
        struct elf_segment* seg = &file->segments[i];
        uintptr_t start = (uintptr_t)seg->vaddr;
        uintptr_t end = start + seg->filesz;

        if (end >= start && vaddr >= start && vaddr <= end && size <= end - vaddr) {
            return (void*)((uintptr_t)seg->paddr + (vaddr - start));
        }
    }

    return NULL;
}

struct elf_file* elf_file_new() {
    return kzalloc(sizeof(struct elf_file));
}

void elf_file_free(struct elf_file* file) {
    if (!file) return;
    if (file->elf_memory) kfree(file->elf_memory);
    for (int i = 0; i < file->needed_count; i++) {
        elf_file_free(file->needed_libraries[i]);
    }
    kfree(file);
}

/**
 * @brief Parses program headers to identify segments and dynamic information.
 */
static status_t elf_process_phdr32(struct elf_file* file, struct elf32_header* header) {
    // First pass: find base address
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr* phdr = elf32_get_phdr(header, i);
        if (phdr->p_type == PT_LOAD) {
            if (!file->base_address) {
                file->base_address = (void*)(uintptr_t)phdr->p_vaddr;
            }
        }
    }
    
    // Second pass: process segments and dynamic section
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr* phdr = elf32_get_phdr(header, i);
        if (phdr->p_type == PT_LOAD) {
            if (file->segment_count >= ELF_MAX_SEGMENTS) return STATUS_ERR(ENOMEM);
            struct elf_segment* seg = &file->segments[file->segment_count++];
            seg->vaddr = (void*)(uintptr_t)phdr->p_vaddr;
            seg->paddr = elf_file_ptr(file, phdr->p_offset, phdr->p_filesz);
            seg->memsz = phdr->p_memsz;
            seg->filesz = phdr->p_filesz;
            seg->flags = phdr->p_flags;
        } else if (phdr->p_type == PT_DYNAMIC) {
            struct elf32_dyn* dyn = elf32_get_dynamic(header, phdr);
            int dyn_count = phdr->p_filesz / sizeof(struct elf32_dyn);
            for (int dyn_index = 0; dyn_index < dyn_count && dyn->d_tag != DT_NULL; dyn_index++, dyn++) {
                switch (dyn->d_tag) {
                    case DT_SYMTAB:    file->dynsym = elf_vaddr_to_file_ptr(file, dyn->d_un.d_ptr, sizeof(struct elf32_sym)); break;
                    case DT_STRTAB:    file->dynstr = elf_vaddr_to_file_ptr(file, dyn->d_un.d_ptr, 1); break;
                    case DT_REL:       file->rel = elf_vaddr_to_file_ptr(file, dyn->d_un.d_ptr, sizeof(struct elf32_rel)); break;
                    case DT_RELSZ:     file->rel_size = dyn->d_un.d_val; break;
                    case DT_RELA:      file->rela = elf_vaddr_to_file_ptr(file, dyn->d_un.d_ptr, sizeof(struct elf32_rela)); break;
                    case DT_RELASZ:    file->rela_size = dyn->d_un.d_val; break;
                    case DT_JMPREL:    file->jmprel = elf_vaddr_to_file_ptr(file, dyn->d_un.d_ptr, sizeof(struct elf32_rel)); break;
                    case DT_PLTRELSZ:  file->jmprel_size = dyn->d_un.d_val; break;
                    case DT_HASH:      file->hash = elf_vaddr_to_file_ptr(file, dyn->d_un.d_ptr, sizeof(uint32_t) * 2); break;
                    case DT_PLTGOT:    file->pltgot = elf_vaddr_to_file_ptr(file, dyn->d_un.d_ptr, sizeof(uint32_t)); break;
                }
            }
        }
    }
    return STATUS_OK;
}

/**
 * @brief Loads an ELF file from disk into memory.
 */
status_t elf_load(const char* filename, struct elf_file** file_out) {
    if (!filename || !file_out) {
        return STATUS_ERR(EINVAL);
    }

    status_t fd = fopen(filename, "rb");
    if (status_is_error(fd)) return fd;

    struct file_stat stat;
    fstat(fd, &stat);
    if (stat.size < sizeof(struct elf32_header) || stat.size > ELF_MAX_FILE_SIZE) {
        fclose(fd);
        return STATUS_ERR(EINVAL);
    }

    // Allocate slightly more for potential unaligned access or stubs
    void* elf_memory = kzalloc(stat.size + 4096);
    if (!elf_memory) {
        fclose(fd);
        return STATUS_ERR(ENOMEM);
    }

    if (fread(elf_memory, 1, stat.size, fd) != stat.size) {
        kfree(elf_memory);
        fclose(fd);
        return STATUS_ERR(EIO);
    }
    fclose(fd);

    status_t res = elf_validate32((struct elf32_header*)elf_memory, stat.size);
    if (status_is_error(res)) {
        kfree(elf_memory);
        return res;
    }

    struct elf_file* file = elf_file_new();
    if (!file) {
        kfree(elf_memory);
        return STATUS_ERR(ENOMEM);
    }

    strncpy(file->filename, filename, sizeof(file->filename));
    file->elf_memory = elf_memory;
    file->in_memory_size = stat.size;

    if (elf_is_32bit(elf_memory)) {
        file->elf_class = ELFCLASS32;
        res = elf_process_phdr32(file, (struct elf32_header*)elf_memory);
        if (status_is_error(res)) {
            elf_file_free(file);
            return res;
        }
    } else {
        elf_file_free(file);
        return STATUS_ERR(EINVAL);
    }

    *file_out = file;
    return STATUS_OK;
}

void elf_close(struct elf_file* file) {
    elf_file_free(file);
}

void* elf_header(struct elf_file* file) {
    return file->elf_memory;
}

struct elf32_header* elf32_header(struct elf_file* file) {
    return (struct elf32_header*)file->elf_memory;
}

struct elf64_header* elf64_header(struct elf_file* file) {
    return (struct elf64_header*)file->elf_memory;
}

void* elf_entry_point(struct elf_file* file) {
    if (file->elf_class == ELFCLASS32) {
        return (void*)(uintptr_t)elf32_header(file)->e_entry;
    }
    return (void*)(uintptr_t)elf64_header(file)->e_entry;
}

/**
 * @brief Standard ELF hash function.
 */
static uint32_t elf_hash(const unsigned char *name) {
    uint32_t h = 0, g;
    while (*name) {
        h = (h << 4) + *name++;
        if ((g = h & 0xf0000000)) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

/**
 * @brief Resolves a virtual address to a physical address within the loaded ELF segments.
 */
static void* elf_vaddr_to_paddr(struct elf_file* file, uintptr_t vaddr) {
    for(int i=0; i < file->segment_count; i++) {
        if (vaddr >= (uintptr_t)file->segments[i].vaddr && 
            vaddr < (uintptr_t)file->segments[i].vaddr + file->segments[i].memsz) {
            return (void*)((uintptr_t)file->segments[i].paddr + (vaddr - (uintptr_t)file->segments[i].vaddr));
        }
    }
    return NULL;
}

/**
 * @brief Performs a symbol lookup within a single ELF file.
 */
static status_t elf_lookup_symbol_in_file(struct elf_file* file, const char* name, void** out) {
    if (file->hash && file->dynsym && file->dynstr) {
        uint32_t* hash_table = (uint32_t*)file->hash;
        uint32_t nbucket = hash_table[0];
        uint32_t* buckets = &hash_table[2];
        uint32_t* chains = &hash_table[2 + nbucket];
        
        uint32_t h = elf_hash((const unsigned char*)name);
        for (uint32_t i = buckets[h % nbucket]; i != 0; i = chains[i]) {
            struct elf32_sym* sym = &((struct elf32_sym*)file->dynsym)[i];
            if (strcmp(file->dynstr + sym->st_name, name) == 0) {
                if (sym->st_shndx != 0) {
                    // Convert symbol virtual address to physical address
                    void* paddr = elf_vaddr_to_paddr(file, sym->st_value);
                    if (paddr) {
                        *out = paddr;
                        return STATUS_OK;
                    }
                }
            }
        }
    } else if (file->dynsym && file->dynstr) {
        struct elf32_sym* syms = (struct elf32_sym*)file->dynsym;
        struct elf32_header* header = elf32_header(file);
        for (int i = 0; i < header->e_shnum; i++) {
            struct elf32_shdr* shdr = elf32_get_shdr(header, i);
            if (shdr->sh_type == SHT_DYNSYM) {
                int count = shdr->sh_size / shdr->sh_entsize;
                for (int j = 0; j < count; j++) {
                    if (strcmp(file->dynstr + syms[j].st_name, name) == 0) {
                        if (syms[j].st_shndx != 0) {
                            // Convert symbol virtual address to physical address
                            void* paddr = elf_vaddr_to_paddr(file, syms[j].st_value);
                            if (paddr) {
                                *out = paddr;
                                return STATUS_OK;
                            }
                        }
                    }
                }
            }
        }
    }
    return STATUS_ERR(ENOENT);
}

/**
 * @brief Performs a global symbol lookup, including dependencies.
 */
status_t elf_lookup_symbol(struct elf_file* file, const char* symbol_name, void** out_addr) {
    if (!file) return STATUS_ERR(EINVAL);
    if (elf_lookup_symbol_in_file(file, symbol_name, out_addr) == STATUS_OK) return STATUS_OK;
    for (int i = 0; i < file->needed_count; i++) {
        if (elf_lookup_symbol(file->needed_libraries[i], symbol_name, out_addr) == STATUS_OK) return STATUS_OK;
    }
    return STATUS_ERR(ENOENT);
}

/**
 * @brief Applies REL type relocations.
 */
static status_t elf_apply_rel(struct elf_file* file, void* rel_ptr, size_t size) {
    if (!rel_ptr) return STATUS_OK;
    struct elf32_rel* rel = (struct elf32_rel*)rel_ptr;
    int count = size / sizeof(struct elf32_rel);
    for (int i = 0; i < count; i++) {
        uint32_t sym_idx = ELF32_R_SYM(rel[i].r_info);
        uint32_t type = ELF32_R_TYPE(rel[i].r_info);
        void* phys_patch_addr = elf_vaddr_to_paddr(file, rel[i].r_offset);
        if (!phys_patch_addr) continue;

        uint32_t* patch_addr = (uint32_t*)phys_patch_addr;
        void* sym_addr = NULL;
        if (sym_idx != 0) {
            struct elf32_sym* syms = (struct elf32_sym*)file->dynsym;
            const char* sym_name = file->dynstr + syms[sym_idx].st_name;
            elf_lookup_symbol(file, sym_name, &sym_addr);
        }

        switch (type) {
            case 1: *patch_addr = (uint32_t)(uintptr_t)sym_addr; break; // R_386_32
            case 2: *patch_addr = (uint32_t)((uintptr_t)sym_addr - rel[i].r_offset); break; // R_386_PC32
            case 6: *patch_addr = (uint32_t)(uintptr_t)sym_addr; break; // R_386_GLOB_DAT
            case 7: *patch_addr = (uint32_t)(uintptr_t)sym_addr; break; // R_386_JMP_SLOT
            case 8: { // R_386_RELATIVE
                // Add load offset: where we're loaded minus where we expect to be
                uintptr_t load_offset = (uintptr_t)file->segments[0].paddr - (uintptr_t)file->segments[0].vaddr;
                *patch_addr += load_offset;
                break;
            }
        }
    }
    return STATUS_OK;
}

/**
 * @brief Applies RELA type relocations.
 */
static status_t elf_apply_rela(struct elf_file* file, void* rela_ptr, size_t size) {
    if (!rela_ptr) return STATUS_OK;
    struct elf32_rela* rela = (struct elf32_rela*)rela_ptr;
    int count = size / sizeof(struct elf32_rela);
    for (int i = 0; i < count; i++) {
        uint32_t sym_idx = ELF32_R_SYM(rela[i].r_info);
        uint32_t type = ELF32_R_TYPE(rela[i].r_info);
        void* phys_patch_addr = elf_vaddr_to_paddr(file, rela[i].r_offset);
        if (!phys_patch_addr) continue;

        uint32_t* patch_addr = (uint32_t*)phys_patch_addr;
        void* sym_addr = NULL;
        if (sym_idx != 0) {
            struct elf32_sym* syms = (struct elf32_sym*)file->dynsym;
            const char* sym_name = file->dynstr + syms[sym_idx].st_name;
            elf_lookup_symbol(file, sym_name, &sym_addr);
        }

        switch (type) {
            case 1: *patch_addr = (uint32_t)((uintptr_t)sym_addr + rela[i].r_addend); break; // R_386_32
            case 2: *patch_addr = (uint32_t)((uintptr_t)sym_addr + rela[i].r_addend - rela[i].r_offset); break; // R_386_PC32
            case 8: { // R_386_RELATIVE
                uintptr_t load_offset = (uintptr_t)file->segments[0].paddr - (uintptr_t)file->segments[0].vaddr;
                *patch_addr = load_offset + rela[i].r_addend;
                break;
            }
        }
    }
    return STATUS_OK;
}

/**
 * @brief Orchestrates the application of all relocations for the file and its dependencies.
 */
status_t elf_apply_relocations(struct elf_file* file) {
    if (!file) return STATUS_ERR(EINVAL);
    elf_apply_rel(file, file->rel, file->rel_size);
    elf_apply_rel(file, file->jmprel, file->jmprel_size);
    elf_apply_rela(file, file->rela, file->rela_size);
    for (int i = 0; i < file->needed_count; i++) {
        elf_apply_relocations(file->needed_libraries[i]);
    }
    return STATUS_OK;
}

/**
 * @brief Recursively loads all necessary libraries for the given ELF.
 */
status_t elf_load_needed_libraries(struct elf_file* file) {
    if (!file || !file->dynstr) return STATUS_OK;
    struct elf32_header* header = elf32_header(file);
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_phdr* phdr = elf32_get_phdr(header, i);
        if (phdr->p_type == PT_DYNAMIC) {
            struct elf32_dyn* dyn = elf32_get_dynamic(header, phdr);
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_NEEDED) {
                    const char* libname = file->dynstr + dyn->d_un.d_val;
                    char path[MAX_PATH];
                    snprintf(path, sizeof(path), "0:/%s", libname);
                    struct elf_file* lib = NULL;
                    if (elf_load(path, &lib) == STATUS_OK) {
                        file->needed_libraries[file->needed_count++] = lib;
                        elf_load_needed_libraries(lib);
                    }
                }
                dyn++;
            }
        }
    }
    return STATUS_OK;
}
