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
static int cake_cleanup(void)
{
  printk( "The code to fail\n" );
  // Non-zero return means that
  // the module couldn't be loaded.
  return -17;
}

//--- unloader -------------------------------------------
static void cake_init(void)
{
  printk( "This code won't be seen\n");
}

//--------------------------------------------------------
module_init(cake_cleanup);
module_exit(cake_init);

//=================== END OF FILE ========================
