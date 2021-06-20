CC = gcc

all: gpiod

gpiod: gpiod_test.c
	$(CC) -l gpiod -o gpiod_test gpiod_test.c

clean:
	rm gpiod_test
