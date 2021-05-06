
/*  
 *  hello.c - Demonstrating the module_init() and module_exit() macros.
 *  This is preferred over using init_module() and cleanup_module().
 */

// REMEMBER: The static keyword restricts the visibility of the function to within this C file.

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_ALERT */
#include <linux/init.h>		/* Needed for the macros, e.g. __init, __exit */

MODULE_LICENSE("GPL");                  // License type
MODULE_AUTHOR("Maíra Canal");           // Author name
MODULE_DESCRIPTION("Simple module with educational intentions"); // Module description 
MODULE_VERSION("0.0.1");                // Module version 

static char *name = "Glauco";          // Default value is "world"
module_param(name, charp, S_IRUGO);    // Param desc. charp = char ptr, S_IRUGO = can be read/not changed
MODULE_PARM_DESC(name, "Name that we want to show");  // parameter description

/** @brief LKM initialization function
 *  The __init macro means that for a built-in driver (not a LKM) the function is only used at 
 * initialization time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init hello_init(void)
{
	printk(KERN_INFO "Hello world, %s\n", name);
	return 0;
}

/** @brief LKM cleanup function
 *  The __exit macro notifies that if this code is used for a built-in driver (not a LKM) 
 *  that this function is not required.
 */
static void __exit hello_exit(void)
{
	printk(KERN_INFO "Goodbye world, %s\n", name);
}
 
/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function
 */
module_init(hello_init);
module_exit(hello_exit);