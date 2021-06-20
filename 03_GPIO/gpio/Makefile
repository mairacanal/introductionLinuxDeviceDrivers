obj-m += gpio_test.o
CC=$(CROSS_COMPILE)gcc

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
