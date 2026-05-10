#ifndef KERNEL_H
#define KERNEL_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file kernel.h
 * @brief Kernel entry points and CPU setup routines.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

/**
 * Enable CPU interrupts globally.
 *
 * @return None.
 */
extern void enable_interrupts();
extern void kernel_idle();

struct paging_desc;
struct paging_desc* kernel_desc();

/**
 * Kernel entry point after low-level boot setup.
 *
 * @return None.
 */
void kernel_main();

/**
 * Kernel registers setup function to initialize segment registers for kernel mode.
 * This function is called during the initial paging setup to ensure that the segment registers are correctly set for kernel execution.
 * 
 */
void kernel_registers();

/**
 * Kernel page setup function to initialize paging for kernel mode.
 * This function is called during the initial paging setup to ensure that the paging structure is correctly set for kernel execution.
 *
 * @return None.
 */
void kernel_page();

#endif /* KERNEL_H */
