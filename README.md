# Introduction to Linux Device Drivers

This repository was created to share all the results of all my studies of Linux Device Drivers. Each folder contains a relevant topic of the development of Linux Device Drivers. 

## How to run it?

You must compile the kernel modules before inserting them. Remember that the kernel modules are dependent on the Linux headers, so you must have them installed. To install them on Debian, you must run:

```
sudo apt install linux-headers-$(uname -r)
```

The kernel modules are dependent on the Linux version and architecture. Except for the first kernel module developed, all the kernel modules developed are ready to be cross-compiled. To cross-compile the module to the Beaglebone Black, you must run:

```
export ARCH=arm
export DTC_FLAGS="-@"
export PATH=~/{path to gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf}/bin/:$PATH
export CROSS_COMPILE=arm-linux-gnueabihf-
```

You must install the Linux appropriate version from the Linux official repository and then run the following commands on the directory:

```
make bb.org_defconfig
make scripts
make modules_prepare
```

Finally, compile the module with:

```
make KDIR=\{path to Linux directory}\linux
```

### Inserting the Module

To load the module at the Beaglebone Black, you must run:
```(shell)
sudo insmod {module name}.ko
```

To unload the module, you must run:
```(shell)
sudo rmmod {module name}.ko
```

To get more information about the module, you can run:
```(shell)
lsmod
modinfo
```

## The Modules

- **01_BasicExample**:  just a famous "Hello World" to get the basics about Linux kernel modules.
- **02_CharDevice**: an example of an important type of kernel module. This module creates a communication path between kernel space and user space through the transmission of chars.
- **03_GPIO**: 3 implementations of GPIO: two in kernel space and one in user space.
    - **gpio**: the simplest implementation of a gpio in kernel space.
    - **gpiod**: an implementation of gpio in user space with the most famous library for gpio.
    - **gpio_kobject**: interfaces gpio through sysfs with kobjects.

