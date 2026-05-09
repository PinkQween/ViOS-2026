#ifndef STATUS_H
#define STATUS_H

#include "stdint.h"

/**
 * Status code type used throughout the kernel.
 *
 * Convention:
 * - 0  : success
 * - >0 : valid non-error values
 * - <0 : error values encoded as -errno
 */

typedef int32_t status_t;

/* =========================
 * Standard error codes
 * (POSIX-like, minimal set)
 * ========================= */

#define EPERM        1   /* Operation not permitted */
#define ENOENT       2   /* No such file or directory */
#define EIO          5   /* I/O error */
#define ENOMEM       12  /* Out of memory */
#define EACCES       13  /* Permission denied */
#define EFAULT       14  /* Bad address */
#define EBUSY        16  /* Device or resource busy */
#define EEXIST       17  /* File exists */
#define ENODEV       19  /* No such device */
#define EINVAL       22  /* Invalid argument */
#define ENOSPC       28  /* No space left */
#define ENOSYS       38  /* Not implemented */
#define ENAMETOOLONG 91  /* File or path name too long */

/* =========================
 * Core helpers
 * ========================= */

/** Successful status value. */
#define STATUS_OK ((status_t)0)

/**
 * Convert an errno-style code to status_t error value.
 *
 * @param e Positive errno-like code.
 * @return Negative status_t error.
 */
#define STATUS_ERR(e) (-(status_t)(e))

/**
 * Extract positive errno-like code from status_t error.
 *
 * @param s status_t value.
 * @return Positive error code for negative statuses.
 */
#define STATUS_CODE(s) (-(s))

/* =========================
 * Checks
 * ========================= */

/**
 * Check whether status is non-error.
 *
 * @param s Status value.
 * @return Non-zero if s >= 0, otherwise 0.
 */
static inline int status_is_ok(status_t s) {
    return s >= 0;
}

/**
 * Check whether status is an error.
 *
 * @param s Status value.
 * @return Non-zero if s < 0, otherwise 0.
 */
static inline int status_is_error(status_t s) {
    return s < 0;
}

/**
 * Check whether status is an error or zero.
 *
 * @param s Status value.
 * @return Non-zero if s <= 0, otherwise 0.
 */
static inline int is_status_error_or_zero(status_t s) {
    return s <= 0;
}

/**
 * Check whether status is exactly success.
 *
 * @param s Status value.
 * @return Non-zero if s == STATUS_OK, otherwise 0.
 */
static inline int status_is_exact_ok(status_t s) {
    return s == 0;
}

/**
 * Convert status code to short string identifier.
 *
 * @param s Status value.
 * @return Constant string label for the status code.
 */
static inline const char* status_to_string(status_t s) {
    if (s == STATUS_OK) return "OK";
    switch (STATUS_CODE(s)) {
        case EPERM: return "EPERM";
        case ENOENT: return "ENOENT";
        case EIO: return "EIO";
        case ENOMEM: return "ENOMEM";
        case EACCES: return "EACCES";
        case EFAULT: return "EFAULT";
        case EBUSY: return "EBUSY";
        case EEXIST: return "EEXIST";
        case ENODEV: return "ENODEV";
        case EINVAL: return "EINVAL";
        case ENOSPC: return "ENOSPC";
        case ENOSYS: return "ENOSYS";
        case ENAMETOOLONG: return "ENAMETOOLONG";
        default: return "UNKNOWN_ERROR";
    }
}

#endif /* STATUS_H */
