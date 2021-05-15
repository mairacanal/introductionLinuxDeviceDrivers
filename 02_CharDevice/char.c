#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define DEVICE_NAME "ebbchar"
#define CLASS_NAME "ebb"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maíra Canal"); 
MODULE_DESCRIPTION("Simple char device for BBB");
MODULE_VERSION("0.0.1");

static int majorNumber;
static char message[256] = {0};
static short size_of_message;
static int numberOpens = 0;
static struct class* ebbcharClass = NULL;
static struct device* ebbcharDevice = NULL;

static DEFINE_MUTEX(mutex);

static int dev_open(struct inode*, struct file*);                        // Called each time the device is opened from user space
static int dev_release(struct inode*, struct file*);                     // Called when the device is closed in user space
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);        // Called when data is sent from the device to user space 
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *); // Caleed when data is sent from the user space to device

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

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

    mutex_init(&mutex);
    printk(KERN_INFO "EBBChar: mutex initialized");

    return 0;

}

static void __exit ebbchar_exit(void) {
    
    device_destroy(ebbcharClass,  MKDEV(majorNumber, 0));
    class_unregister(ebbcharClass);
    class_destroy(ebbcharClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    mutex_destroy(&mutex);
    printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");

}

/** @brief The device open function that is called each time the device is opened
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode* inodep, struct file* filep){

    if (!mutex_trylock(&mutex)) {
        printk(KERN_ALERT "EBBChar: device in use by another process\n");
        return -EBUSY; 
    }
    
    numberOpens++;
    printk(KERN_INFO "EBBChar: device has been opened %d time(s)\n", numberOpens);
    return 0;

}

static int dev_release(struct inode* inodep, struct file* filep) {

    mutex_unlock(&mutex);
    printk(KERN_INFO "EBBChar: device successfully closed\n");
    return 0;

}

/** @brief This function is called whenever data is being sent from the device to the 
 *  user. In this case is uses the copy_to_user() function to send the buffer string
 *  to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;

    // copy_to_user has the args (*to, *from, size) and returns 0 on success
    error_count = copy_to_user(buffer, message, size_of_message);

    if (!error_count) {
        printk(KERN_INFO "EBBChar: sent %d characters to the user\n", size_of_message);
        return (size_of_message=0);
    }

    printk(KERN_INFO "EBBChar: failed to send %d characters to the user\n", error_count);
    return -EFAULT;
}   

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    copy_from_user(message, buffer, len);
    size_of_message = len;
    printk(KERN_INFO "EBBChar: received %zu characters from the user\n", len);
    return len;
}

module_init(ebbchar_init);
module_exit(ebbchar_exit);

