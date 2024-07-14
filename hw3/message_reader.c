#include "message_slot.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void check(int value) {
    if (value < 0) {
        perror("message_reader");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "message_reader: unexpected number of arguments\n");
        return 1;
    }
    int slot_fd = open(argv[1], O_RDONLY);
    check(slot_fd);
    int id = atoi(argv[2]);
    printf("ioctling to id %d\n", id);
    check(ioctl(slot_fd, MSG_SLOT_CHANNEL, atoi(argv[2])));
    char buffer[128];
    int bytes_read;
    check(bytes_read = read(slot_fd, buffer, 128));
    check(close(slot_fd));
    printf("bytes read: %d\n", bytes_read);
    check(write(STDOUT_FILENO, buffer, bytes_read));
    return 0;
}
