obj-m += char.o
CC = $(CROSS_COMPILE)gcc

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC) userchar.c -o userchar

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm userchar
