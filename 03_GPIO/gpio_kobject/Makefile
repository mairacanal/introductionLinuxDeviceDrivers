obj-m += button_kobject.o
CC=$(CROSS_COMPILE)gcc

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
