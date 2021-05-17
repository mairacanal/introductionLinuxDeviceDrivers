#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#define DEVICE_NAME "ebbchar"
#define CLASS_NAME "ebb"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maíra Canal"); 
MODULE_DESCRIPTION("Simple char device for BBB");
MODULE_VERSION("0.0.1");

static int majorNumber;
static int numberOpens = 0;
static struct class* ebbcharClass = NULL;
static struct device* ebbcharDevice = NULL;

static DEFINE_SEMAPHORE(semaphore);

static int dev_open(struct inode*, struct file*);                        // Called each time the device is opened from user space
static int dev_release(struct inode*, struct file*);                     // Called when the device is closed in user space
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);        // Called when data is sent from the device to user space 
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *); // Caleed when data is sent from the user space to device

/** @brief Devices are represented as file structure in the kernel. The struct file_operations from /linux/fs.h 
 *  lists the callback functions associated to the file operations
 */
static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

/** @brief LKM initialization function
 *  Function used at initialization time and is responsable for allocating dynamically
 *  the major number, registering device class, device driver and semaphore.
 *  @return returns 0 if successful
 */
static int __init ebbchar_init(void) {

    printk(KERN_INFO "EBBChar: initializing the EBBChar LKM\n");

    // Dynamically allocate a major number for a device
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);

    if (majorNumber < 0) {
        printk(KERN_ALERT "EBBChar: failed to register major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "EBBChar: registered correctly with major number\n");

    // Register the device class
    ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(ebbcharClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "EBBChar: failed to register device class\n");
        return PTR_ERR(ebbcharClass);
    }
    printk(KERN_INFO "EBBChar: device class registered correctly\n");

    // Register the device driver
    ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(ebbcharDevice)) {
        class_destroy(ebbcharClass);
        class_unregister(ebbcharClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "EBBChar: failed to create the device\n");
        return PTR_ERR(ebbcharDevice);
    }
    printk(KERN_INFO "EBBChar: device class created correctly\n");

    sema_init(&semaphore, 5);
    printk(KERN_INFO "EBBChar: semaphore initialized");

    return 0;

}

/** @brief LKM cleanup function
 *  Function responsable for destroying all classes, devices and major number
 */
static void __exit ebbchar_exit(void) {
    
    device_destroy(ebbcharClass,  MKDEV(majorNumber, 0));     // remove the device
    class_unregister(ebbcharClass);                           // unregister the device class
    class_destroy(ebbcharClass);                              // remove the device class
    unregister_chrdev(majorNumber, DEVICE_NAME);              // unregister the major number

    printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");

}

/** @brief The device open function that is called each time the device is opened
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @return returns 0 if successful
 */
static int dev_open(struct inode* inodep, struct file* filep){

    // Tries to hold a semaphore
    if (down_trylock(&semaphore) != 0) {
        printk(KERN_ALERT "EBBChar: device in use by another process\n");
        return -EBUSY; 
    }
    
    numberOpens++;
    printk(KERN_INFO "EBBChar: device has been opened %d time(s)\n", numberOpens);
    return 0;

}

/** @brief The device release function that is called each time the device is released 
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @return returns 0 if successful
 */
static int dev_release(struct inode* inodep, struct file* filep) {

    // Realeases a semaphore
    up(&semaphore);
    printk(KERN_INFO "EBBChar: device successfully closed\n");
    return 0;

}

/** @brief This function is called whenever data is being sent from the device to the 
 *  user. In this case is uses the copy_to_user() function to send the buffer string
 *  to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the buffer
 *  @param offset The offset if required
 *  @return returns 0 if successful
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;

    // copy_to_user has the args (*to, *from, size) and returns 0 on success
    error_count = copy_to_user(buffer, filep->private_data, strlen(filep->private_data));

    if (!error_count) {
        printk(KERN_INFO "EBBChar: sent %d characters to the user\n", (int) strlen(filep->private_data));
        kfree(filep->private_data);
        return 0;
    }

    printk(KERN_INFO "EBBChar: failed to send %d characters to the user\n", error_count);
    return -EFAULT;
}   

/** @brief This function is called whenever data is being sent from the user to the 
 *  device. In this case is uses the copy_from_user() function to receive the buffer string
 *  from the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the buffer
 *  @param offset The offset if required
 *  @return returns 0 if successful
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {

    filep->private_data = (char *) kmalloc(sizeof(char) * len, GFP_KERNEL);

    // copy_from_user has the args (*to, *from, size) and returns 0 on success
    copy_from_user(filep->private_data, buffer, len);
    printk(KERN_INFO "EBBChar: received %zu characters from the user\n", len);
    return len;
}

module_init(ebbchar_init);
module_exit(ebbchar_exit);

