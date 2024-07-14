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
#include </usr/src/linux-headers-5.15.0-113/include/linux/module.h>
// included for __init and __exit macros
#include </usr/src/linux-headers-5.15.0-113/include/linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your TA");
MODULE_DESCRIPTION("Show the different message levels of printk");

//--- loader ---------------------------------------------
static __init int hello_init(void) {
    printk("msg: Default\n");
    printk(KERN_DEBUG "msg: Debug\n");
    printk(KERN_INFO "msg: Info\n");
    printk(KERN_NOTICE "msg: Notice\n");
    printk(KERN_WARNING "msg: Warning\n");
    printk(KERN_ERR "msg: Error\n");
    printk(KERN_CRIT "msg: Critical\n");
    printk(KERN_EMERG "msg: EMERGENCY!!\n");
    // Non-zero return means that
    // the module couldn't be loaded.
    return 0;
}

//--- unloader -------------------------------------------
static void __exit hello_cleanup(void) { printk("Cleaning up msg module.\n"); }

//--------------------------------------------------------
module_init(hello_init);
module_exit(hello_cleanup);

//=================== END OF FILE ========================
