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
        perror("message_sender");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "message_reader: unexpected number of arguments\n");
        return 1;
    }
    int slot_fd = open(argv[1], O_WRONLY);
    check(slot_fd);
    check(ioctl(slot_fd, MSG_SLOT_CHANNEL, atoi(argv[2])));
    char buffer[128];
    int bytes_read;
    check(bytes_read = read(slot_fd, buffer, 128));
    check(close(slot_fd));
    check(write(STDOUT_FILENO, buffer, bytes_read));
    return 0;
}
