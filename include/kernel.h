#ifndef KERNEL_H
#define KERNEL_H

/**
 * Enable CPU interrupts globally.
 *
 * @return None.
 */
extern void enable_interrupts();
extern void kernel_idle();

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
