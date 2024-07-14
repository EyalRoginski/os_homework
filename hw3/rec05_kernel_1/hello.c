// Declare what kind of code we want from
// the header files.
// Defining __KERNEL__ and MODULE allows us
// to access kernel-level code.
// --------------------------------------------------------

// We are part of the kernel
#undef __KERNEL__
#define __KERNEL__

// Not for long
#undef MODULE
#define MODULE

// included for all kernel modules
#include <linux/module.h>
// included for __init and __exit macros
#include <linux/init.h>

MODULE_LICENSE("GPL");

//--- loader ---------------------------------------------
static int hello_init(void) {
    printk("Hello, Kernel World!\n");
    // Non-zero return means that
    // the module couldn't be loaded.
    return 0;
}

//--- unloader -------------------------------------------
static void hello_cleanup(void) { printk("Cleaning up hello module.\n"); }

//--------------------------------------------------------
module_init(hello_init);
module_exit(hello_cleanup);

//=================== END OF FILE ========================
