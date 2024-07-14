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
MODULE_AUTHOR("ERT");
MODULE_DESCRIPTION("Panic!");

//--- loader ---------------------------------------------
static __init int panic_init(void)
{
  panic("PANIC!!!!");
  return 0;
}

//--- unloader -------------------------------------------
static void __exit panic_cleanup(void)
{
  printk( "Good luck getting here\n");
}

//--------------------------------------------------------
module_init(panic_init);
module_exit(panic_cleanup);

//=================== END OF FILE ========================
