#include "fs/pparser.h"
#include "string/string.h"
#include "config.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"

#include "stdbool.h"

static bool pathparser_is_valid_format(const char* filepath)
{
    if (!filepath || strnlen(filepath, MAX_PATH) == 0) {
        return false;
    }

    size_t i = 0;
    if (!is_digit(filepath[i])) {
        return false;
    }

    while (is_digit(filepath[i])) {
        i++;
    }

    return filepath[i] == ':' && filepath[i + 1] == '/';
}

static int pathparser_get_drive_number_by_path(const char** filepath)
{
    if (!pathparser_is_valid_format(*filepath))
    {
        return STATUS_ERR(EINVAL);
    }

    int drive_number = 0;
    size_t offset = 0;

    while (is_digit((*filepath)[offset])) {
        drive_number = (drive_number * 10) + ((*filepath)[offset] - '0');
        offset++;
    }

    offset += 2;

    *filepath += offset;

    return drive_number;
}

static struct path_root* pathparser_create_root(int drive_number)
{
    struct path_root* root_path = kmalloc(sizeof(struct path_root));
    
    if (!root_path) {
        return NULL;
    }

    root_path->drive_number = drive_number;
    root_path->first = NULL;

    return root_path;
}

static char* pathparser_get_next_part(const char** filepath, bool* success)
{
    *success = true;

    const char* start = *filepath;

    while (**filepath != '/' && **filepath != '\0') {
        (*filepath)++;
    }

    size_t part_length = (size_t)(*filepath - start);
    if (part_length == 0) {
        return NULL;
    }

    char* next_part = kmalloc(part_length + 1);
    if (!next_part) {
        *success = false;
        return NULL;
    }

    for (size_t i = 0; i < part_length; i++) {
        next_part[i] = start[i];
    }

    next_part[part_length] = '\0';

    if (**filepath == '/') {
        (*filepath)++;
    }

    return next_part;
}

static struct path_part* pathparser_parse_path_part(struct path_part* current_part, const char** filepath, bool* success)
{
    char* next_part_name = pathparser_get_next_part(filepath, success);

    if (!next_part_name) {
        return NULL;
    }

    struct path_part* next_part = kzalloc(sizeof(struct path_part));
    if (!next_part) {
        kfree(next_part_name);
        *success = false;
        return NULL;
    }

    next_part->name = next_part_name;
    next_part->next = NULL;

    if (current_part) {
        current_part->next = next_part;
    }

    return next_part;
}

static void pathparser_free_partial(struct path_root* root)
{
    pathparser_free(root);
}

void pathparser_free(struct path_root* root)
{
    if (!root) {
        return;
    }

    struct path_part* current_part = root->first;

    while (current_part) {
        struct path_part* next_part = current_part->next;

        kfree((void*)current_part->name);
        kfree(current_part);

        current_part = next_part;
    }

    kfree(root);
}

status_t pathparser_parse_ex(const char* filepath, const char* current_working_directory, struct path_root** root_out)
{
    status_t res = STATUS_OK;

    const char* tmp_path = filepath;
    struct path_root* root = NULL;

    if (!root_out) {
        return STATUS_ERR(EFAULT);
    }

    *root_out = NULL;

    if (strlen(filepath) > MAX_PATH) {
        return STATUS_ERR(EINVAL);
    }

    res = pathparser_get_drive_number_by_path(&tmp_path);

    if (res < 0) {
        return STATUS_ERR(EINVAL);
    }

    root = pathparser_create_root(res);

    if (!root) {
        return STATUS_ERR(ENOMEM);
    }

    bool success = true;
    struct path_part* current_part = pathparser_parse_path_part(NULL, &tmp_path, &success);
    if (!success) {
        pathparser_free_partial(root);
        return STATUS_ERR(ENOMEM);
    }

    if (!current_part) {
        pathparser_free_partial(root);
        return STATUS_ERR(EINVAL);
    }

    root->first = current_part;

    struct path_part* next_part = pathparser_parse_path_part(current_part, &tmp_path, &success);

    while (next_part) {
        current_part = next_part;

        next_part = pathparser_parse_path_part(current_part, &tmp_path, &success);
        if (!success) {
            pathparser_free_partial(root);
            return STATUS_ERR(ENOMEM);
        }
    }

    *root_out = root;
    return STATUS_OK;
}

struct path_root* pathparser_parse(const char* filepath, const char* current_working_directory)
{
    struct path_root* root = NULL;

    if (pathparser_parse_ex(filepath, current_working_directory, &root) < 0) {
        return NULL;
    }

    return root;
}