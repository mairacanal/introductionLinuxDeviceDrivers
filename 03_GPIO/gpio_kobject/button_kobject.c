/*
 * @file button_kobject.c
 * @author Maíra Canal
 * @date 24 May 2021
 * @brief A kernel module for controlling a button, connected to a GPIO.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/ktime.h>
#include <linux/sysfs.h>

#define DEBOUNCE 200
#define RW_MODE  0664

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maíra Canal");
MODULE_DESCRIPTION("A kernel module for controlling a button, connected to a GPIO.");
MODULE_VERSION("0.0.1");

static bool isRising = 1;
module_param(isRising, bool, S_IRUGO);
MODULE_PARM_DESC(isRising, "Rising edge = 1 (default); Falling edge = 0");

static unsigned int gpioButton = 49;
module_param(gpioButton, uint, S_IRUGO);
MODULE_PARM_DESC(gpioButton, "GPIO Button number (default = 49)");

static unsigned int gpioLED = 115;
module_param(gpioLed, uint, S_IRUGO);
MODULE_PARM_DESC(gpioLed, "GPIO LED number (default = 115)");

static char gpioName[8];
static unsigned int irqNum;
static unsigned int numberPresses = 0;
static bool ledValue = 0;
static unsigned int isDebounce = 1;
static ktime_t t_last, t_current, t_diff;

// ******************************************************************************************* Functions prototypes

/*  
 *  @brief Handle the interrupt on the button's gpio pin
 *  @param irq The interrupt number
 *  @param dev_id The dev_id registered at request_irq() 
 *  @param regs The value of registers before being interrupted 
 *  @return returns the irq_handler_t struct
 */

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/*  
 *  @brief Shows the number of presses at sysfs
 *  @param kobj Kobject associated to the function
 *  @param attr Struct kobj_attribute associated to the function
 *  @param buf Buffer from sysfs
 *  @return returns the size of what was written in buffer 
 */

static ssize_t numberPresses_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

/*  
 *  @brief Stores the number of presses in sysfs
 *  @param kobj Kobject associated to the function
 *  @param attr Struct kobj_attribute associated to the function
 *  @param buf Buffer from sysfs
 *  @return returns the size of what was read in buffer 
 */

static ssize_t numberPresses_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

/*  
 *  @brief Shows the boolean state of the led in sysfs
 *  @param kobj Kobject associated to the function
 *  @param attr Struct kobj_attribute associated to the function
 *  @param buf Buffer from sysfs
 *  @return returns the size of what was read in buffer 
 */

static ssize_t ledValue_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

/*  
 *  @brief Shows the last time the interrupt was triggered 
 *  @param kobj Kobject associated to the function
 *  @param attr Struct kobj_attribute associated to the function
 *  @param buf Buffer from sysfs
 *  @return returns the size of what was read in buffer 
 */

static ssize_t lastTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

/*  
 *  @brief Shows the time between two interrupts
 *  @param kobj Kobject associated to the function
 *  @param attr Struct kobj_attribute associated to the function
 *  @param buf Buffer from sysfs
 *  @return returns the size of what was read in buffer 
 */

static ssize_t diffTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

/*  
 *  @brief Shows the boolean state of the debounce in sysfs
 *  @param kobj Kobject associated to the function
 *  @param attr Struct kobj_attribute associated to the function
 *  @param buf Buffer from sysfs
 *  @return returns the size of what was read in buffer 
 */

static ssize_t isDebounce_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

/*  
 *  @brief Stores the boolean state of the debounce in sysfs
 *  @param kobj Kobject associated to the function
 *  @param attr Struct kobj_attribute associated to the function
 *  @param buf Buffer from sysfs
 *  @return returns the size of what was read in buffer 
 */

static ssize_t isDebounce_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

// ****************************************************************************************************************

// Using helper macros to define the name and access levels of the kobj_attributes
static struct kobj_attribute count_attr = __ATTR(numberPresses, RW_MODE, numberPresses_show, numberPresses_store);
static struct kobj_attribute debounce_attr = __ATTR(isDebounce, RW_MODE, isDebounce_show, isDebounce_store);
static struct kobj_attribute led_attr = __ATTR(ledValue, S_IRUGO, ledValue_show, NULL);
static struct kobj_attribute time_attr = __ATTR(lastTime, S_IRUGO, lastTime_show, NULL);
static struct kobj_attribute diff_attr = __ATTR(diffTime, S_IRUGO, diffTime_show, NULL);

// Array of attributes to create a group of attributes
static struct attribute *attrs[] = {&count_attr.attr, &debounce_attr.attr, &led_attr.attr, &time_attr.attr, &diff_attr.attr, NULL};

// This attribute array and name will be exposed on sysfs
static struct attribute_group attr_group = {
    .name = gpioName,
    .attrs = attrs,
};

static struct kobject *gpio_kobj;

static int __init button_init(void) {

    int result = 0;
    unsigned long IRQflag = IRQF_TRIGGER_RISING;

    printk(KERN_INFO "BUTTON: initializing the BUTTON LKM\n");
    sprintf(gpioName, "gpio%d", gpioButton);

    // Creating the kobject at /sys/kernel
    gpio_kobj = kobject_create_and_add("button", kernel_kobj);
    if (!gpio_kobj) {
        printk(KERN_INFO "BUTTON: error creating kobject\n");
        return -ENOMEM;
    }

    // Instantiating the kobject with the attributes of attr_group
    result = sysfs_create_group(gpio_kobj, &attr_group);
    if (result) {
        printk(KERN_ALERT "BUTTON: failed creating sysfs group\n");
        kobject_put(gpio_kobj);
        return result;
    }

    // Instantiating the time instances
    t_last = ktime_get_real();
    t_diff = ktime_sub(t_last, t_last);

    // Requesting the LED gpio pin and setting it to output
    gpio_request(gpioLed, "sysfs");
    gpio_direction_output(gpioLed, ledValue);
    gpio_export(gpioLed, false);

    // Requesting the button gpio pin, setting debounce and setting it to input
    gpio_request(gpioButton, "sysfs");
    gpio_direction_input(gpioButton);
    gpio_set_debounce(gpioButton, DEBOUNCE);
    gpio_export(gpioButton, false);

    // Mapping GPIO to IRQ 
    irqNum = gpio_to_irq(gpioButton);
    printk(KERN_INFO "BUTTON: the button is mapped to IRQ: %d\n", irqNum);

    if (!isRising) IRQflag = IRQF_TRIGGER_FALLING;

    // Requesting interrupt
    result = request_irq(irqNum, (irq_handler_t) gpio_irq_handler, IRQflag, "button_handler", NULL);

    return result;

}

static void __exit button_exit(void) {

    printk(KERN_INFO "BUTTON: the button was pressed %d times\n", numberPresses);

    // Desallocating the kobject
    kobject_put(gpio_kobj);

    // Turn LED off
    gpio_set_value(gpioLed, 0);

    // Freeing gpio and interrupt 
    gpio_unexport(gpioLed);
    free_irq(irqNum, NULL);
    gpio_unexport(gpioButton);
    gpio_free(gpioLed);
    gpio_free(gpioButton);

    printk(KERN_INFO "BUTTON: LKM removed successfully\n");

}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {

    // Toggle LED
   ledValue = !ledValue;
   gpio_set_value(gpioLed, ledValue);

   // Time log
   t_current = ktime_get_real();
   t_diff = ktime_sub(t_current, t_last);
   t_last = t_current;
   numberPresses++;

   return (irq_handler_t) IRQ_HANDLED;

}

static ssize_t numberPresses_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", numberPresses);
}

static ssize_t numberPresses_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%d", &numberPresses);
    return count;
}

static ssize_t ledValue_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", ledValue);
}

static ssize_t lastTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%.9llu\n", ktime_to_ns(t_last));
}

static ssize_t diffTime_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%.9llu\n", ktime_to_ns(t_diff));
}

static ssize_t isDebounce_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", isDebounce);
}

static ssize_t isDebounce_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%d", &isDebounce);

    if (isDebounce) {
        gpio_set_debounce(gpioButton, DEBOUNCE);
        printk(KERN_INFO "BUTTON: Debounce on\n");
    } else {
        gpio_set_debounce(gpioButton, 0);
        printk(KERN_INFO "BUTTON: Debounce off\n");
    }

    return count;
}

module_init(button_init);
module_exit(button_exit);
