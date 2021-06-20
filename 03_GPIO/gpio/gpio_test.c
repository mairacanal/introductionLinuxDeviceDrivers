/*
 * @file gpio_test.c
 * @author Maíra Canal
 * @date 22 may 2021
 * @brief A kernel module for controlling a GPIO LED/button pair
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maíra Canal");
MODULE_DESCRIPTION("A kernel module for controlling a GPIO LED/button pair");
MODULE_VERSION("0.0.1");

static unsigned int gpioLed = 49;
static unsigned int gpioButton = 115;
static unsigned int irqNum;
static unsigned int value = 0;

/*  
 *  @brief Handle the interrupt on the button's gpio pin
 *  @param irq The irq number
 *  @param dev_id The dev_id registered at request_irq() 
 *  @param regs The value of registers before being interrupted 
 *  @return returns 0 if successful
 */

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int __init LEDgpio_init (void) {

    int result = 0;

    printk(KERN_INFO "GPIO_TEST: initializing the LKM GPIO_TEST\n");

    // Checking if the gpioLed is valid
    if(!gpio_is_valid(gpioLed)) {
        printk(KERN_INFO "GPIO_TEST: invalid GPIO\n");
        return -ENODEV;
    }

    // Requesting the LED gpio pin and setting it to output
    value  = 1;
    gpio_request(gpioLed, "sysfs");
    gpio_direction_output(gpioLed, value);
    gpio_export(gpioLed, false);

    // Requesting the button gpio pin, setting debounce and setting it to input
    gpio_request(gpioButton, "sysfs");
    gpio_direction_input(gpioButton);
    gpio_set_debounce(gpioButton, 200);
    gpio_export(gpioButton, false);

    printk(KERN_INFO "GPIO_TEST: GPIO configured\n");

    // Mapping GPIO to IRQ and "connecting" them
    irqNum = gpio_to_irq(gpioButton);
    printk(KERN_INFO "GPIO_TEST: the button is mapped to IRQ #%d\n", irqNum);

    // Requesting interrupt in Rising Edge 
    result = request_irq(irqNum, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING, "gpio_handler", NULL);

    printk(KERN_INFO "GPIO_TEST: the interrupt request resulted %d", result);
    return result;

}

static void __exit LEDgpio_exit (void) {

    // Turn LED off
    gpio_set_value(gpioLed, 0);

    // Freeing gpio and interrupt number
    gpio_unexport(gpioLed);
    gpio_unexport(gpioButton);

    free_irq(irqNum, NULL);
    gpio_free(gpioLed);
    gpio_free(gpioButton);

    printk(KERN_INFO "GPIO_TEST: Exiting GPIO_TEST LKM\n");

}

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {

    // Toggle LED
    value = !value;    
    gpio_set_value(gpioLed, value);
    return (irq_handler_t) IRQ_HANDLED;

}

module_init(LEDgpio_init);
module_exit(LEDgpio_exit);
