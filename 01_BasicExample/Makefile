## obj-m: defines a loadable module goal
obj-m += hello.o

## $(shell uname -r): return the current kernel build version
## -C: switches the directory to the kernel directory 
## M=$(PWD): tells the make command where the actual project files exist
## modules: default target for external kernel modules

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
