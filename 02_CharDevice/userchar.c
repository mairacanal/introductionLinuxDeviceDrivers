#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static char* receive = NULL;

char* read_string();

int main(void) {
    int ret, fd;
    char* stringToSend = NULL;

    printf("Starting...\n");
    fd = open("/dev/ebbchar", O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device");
        return errno;
    }

    printf("Type in a short string to send to the kernel module:\n");
    // scanf("%[^\n]%*c", stringToSend);
    stringToSend = read_string();
    printf("Writing message to the device [%s].\n", stringToSend);
    ret = write(fd, stringToSend, strlen(stringToSend));
    if (ret < 0) {
        perror("Failed to write the message to the device.");
        return errno;
    }

    printf("Press ENTER to read back from the device.\n");
    getchar();

    receive = malloc(sizeof(char) * strlen(stringToSend));
    printf("Reading from the device.\n");
    ret = read(fd, receive, strlen(stringToSend));
    if (ret < 0) {
        perror("Failed to read the message to the device.");
        return errno;
    }
    printf("The received message is [%s]\n", receive);

    free(stringToSend);
    free(receive);

    return 0;
    
}

char* read_string() {

    char *str = (char*) malloc(sizeof(char));
    char c = ' ';
    int count = 0;

    while ((c = getc(stdin)) && c != '\n') {

        str = (char*) realloc(str, (count + 1) * sizeof(char));
        str[count++] = c;

    }

    return str;

}
