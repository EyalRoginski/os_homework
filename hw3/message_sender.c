#include "message_slot.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void check(int value) {
    if (value < 0) {
        perror("message_sender");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "message_sender: unexpected number of arguments\n");
        return 1;
    }
    int slot_fd = open(argv[1], O_WRONLY);
    check(slot_fd);
    check(ioctl(slot_fd, MSG_SLOT_CHANNEL, atoi(argv[2])));
    printf("sending %ld bytes: %s\n", strlen(argv[3]), argv[3]);
    check(write(slot_fd, argv[3], strlen(argv[3])));
    check(close(slot_fd));
    return 0;
}
