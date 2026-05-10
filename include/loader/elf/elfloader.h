#ifndef ELFLOADER_H
#define ELFLOADER_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file elfloader.h
 * @brief ELF loader interface for kernel process startup.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "status.h"

#include "stddef.h"
#include "stdint.h"

#define ELFCLASS32 1
#define ELFCLASS64 2

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_SONAME   14
#define DT_RPATH    15
#define DT_SYMBOLIC 16
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_DEBUG    21
#define DT_TEXTREL  22
#define DT_JMPREL   23

#define ELF_MAX_NEEDED_LIBS 256
#define ELF_MAX_SEGMENTS 32

struct elf_segment {
    void* vaddr;
    void* paddr;
    size_t memsz;
    size_t filesz;
    uint32_t flags;
};

struct elf_file {
    char filename[128];
    void* elf_memory; // Raw file data
    size_t in_memory_size;
    int elf_class;

    struct elf_segment segments[ELF_MAX_SEGMENTS];
    int segment_count;
    void* base_address; // Base virtual address of first LOAD segment

    void* entry;
    
    // Dynamic section data
    void* dynsym;
    char* dynstr;
    void* rel;
    size_t rel_size;
    void* rela;
    size_t rela_size;
    void* jmprel;
    size_t jmprel_size;
    void* hash;
    void* pltgot;

    struct elf_file* needed_libraries[ELF_MAX_NEEDED_LIBS];
    int needed_count;
};

/**
 * Allocate and initialize an ELF file descriptor.
 *
 * @return New ELF file descriptor, or NULL on allocation failure.
 */
struct elf_file* elf_file_new();

/**
 * Free an ELF file descriptor and owned backing memory.
 *
 * @param file ELF file descriptor to free.
 * @return None.
 */
void elf_file_free(struct elf_file* file);

/**
 * Load an ELF file from the filesystem.
 *
 * @param filename Path to the ELF file.
 * @param file_out Output pointer for the loaded ELF descriptor.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t elf_load(const char* filename, struct elf_file** file_out);

/**
 * Close and free a loaded ELF file descriptor.
 *
 * @param file ELF file descriptor to close.
 * @return None.
 */
void elf_close(struct elf_file* file);

/**
 * Load dynamic libraries referenced by a loaded ELF file.
 *
 * @param file Loaded ELF file descriptor.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t elf_load_needed_libraries(struct elf_file* file);

/**
 * Apply dynamic relocations for a loaded ELF file.
 *
 * @param file Loaded ELF file descriptor.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t elf_apply_relocations(struct elf_file* file);

/**
 * Resolve a symbol address from a loaded ELF file and its dependencies.
 *
 * @param file Loaded ELF file descriptor.
 * @param symbol_name Symbol name to resolve.
 * @param out_addr Output pointer for the resolved address.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t elf_lookup_symbol(struct elf_file* file, const char* symbol_name, void** out_addr);

/**
 * Get the native ELF header pointer for a loaded file.
 *
 * @param file Loaded ELF file descriptor.
 * @return Pointer to the ELF header.
 */
void* elf_header(struct elf_file* file);

/**
 * Get the ELF32 header for a loaded file.
 *
 * @param file Loaded ELF file descriptor.
 * @return ELF32 header pointer, or NULL if the file is not ELF32.
 */
struct elf32_header* elf32_header(struct elf_file* file);

/**
 * Get the ELF64 header for a loaded file.
 *
 * @param file Loaded ELF file descriptor.
 * @return ELF64 header pointer, or NULL if the file is not ELF64.
 */
struct elf64_header* elf64_header(struct elf_file* file);

/**
 * Get the executable entry point for a loaded ELF file.
 *
 * @param file Loaded ELF file descriptor.
 * @return Entry point pointer.
 */
void* elf_entry_point(struct elf_file* file);

#endif // ELFLOADER_H
