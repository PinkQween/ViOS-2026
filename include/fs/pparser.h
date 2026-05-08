#ifndef PATH_PARSER_H
#define PATH_PARSER_H

#include "status.h"

/** Parsed absolute path root including drive number. */
struct path_root {
    /** Numeric drive id parsed from leading N:/ prefix. */
    int drive_number;
    /** Head of linked list path components. */
    struct path_part* first;
};

/** Single parsed path component in a linked list chain. */
struct path_part {
    /** Component text (heap-allocated, null-terminated). */
    const char* name;
    /** Next component, or NULL for the last element. */
    struct path_part* next;
};

/**
 * Free an entire parsed path tree and all component names.
 *
 * @param root Parsed path root to free.
 * @return None.
 */
void pathparser_free(struct path_root* root);

/**
 * Parse a path string into a structured root and part list.
 *
 * @param filepath Input path, e.g. 0:/hello.txt.
 * @param current_working_directory Reserved for relative-path support.
 * @param root_out Output parsed structure on success.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t pathparser_parse_ex(const char* filepath, const char* current_working_directory, struct path_root** root_out);

/**
 * Parse a path string and return a heap-allocated parsed root.
 *
 * @return Parsed root or NULL on error.
 */
struct path_root* pathparser_parse(const char* filepath, const char* current_working_directory);

#endif /* PATH_PARSER_H */