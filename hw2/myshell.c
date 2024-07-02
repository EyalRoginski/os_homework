#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

int child1 = -1;
int child2 = -1;

void sigint_handler(int signal) {
    if (child1 > 0) {
        kill(child1, SIGINT);
    }
    if (child2 > 0) {
        kill(child2, SIGINT);
    }
    // So `^C` doesn't show up at the start of the next line, but causes a
    // linebreak instead
    printf("\n");
}

int prepare() {
    struct sigaction sigint_action;
    memset(&sigint_action, 0, sizeof(sigint_action));
    sigint_action.sa_handler = sigint_handler;
    sigint_action.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sigint_action, NULL) == -1) {
        perror("signal");
    }
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal");
    }
    return 0;
}

/**
 * Find `word` in `wordlist`, and return the index of its first occurance.
 * Returns -1 if not found.
 * */
int find(int count, char **wordlist, char *word) {
    for (int i = 0; i < count; i++) {
        if (strcmp(wordlist[i], word) == 0) {
            return i;
        }
    }
    return -1;
}

void checked_exec(char **arglist) {
    if (execvp(arglist[0], arglist) == -1) {
        perror("exec");
        exit(1);
    }
}

/**
 * Run two binaries according to the arglists, and pipe between them (1 | 2).
 * */
int pipe_between(char **arglist1, char **arglist2) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    child1 = fork();
    if (child1 < 0) {
        perror("fork");
        return 1;
    }
    if (child1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        checked_exec(arglist1);
        // Should not return
    }
    child2 = fork();
    if (child2 < 0) {
        perror("fork");
        return 1;
    }
    if (child2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        checked_exec(arglist2);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}

/**
 * Execute a binary with `dup_to` redirected to `fd`, and close `fd` in both the
 * parent and the child.
 * */
int redirection(char **arglist, int fd, int dup_to) {
    child1 = fork();
    if (child1 < 0) {
        perror("fork");
        return 1;
    }
    if (child1 == 0) {
        dup2(fd, dup_to);
        close(fd);
        checked_exec(arglist);
    }
    close(fd);
    return 0;
}

int process_arglist(int count, char **arglist) {
    int index;
    if (strcmp(arglist[count - 1], "&") == 0) {
        // Run in background
        arglist[count - 1] = NULL;
        int child = fork();
        if (child < 0) {
            perror("fork");
            return 1;
        }
        if (child == 0) {
            checked_exec(arglist);
        }
    } else if ((index = find(count, arglist, "|")) != -1) {
        arglist[index] = NULL;
        pipe_between(arglist, &arglist[index + 1]);
    } else if ((index = find(count, arglist, "<")) != -1) {
        int fd = open(arglist[index + 1], 0);
        if (fd == -1) {
            perror("file");
            return 1;
        }
        arglist[index] = NULL;

        redirection(arglist, fd, STDIN_FILENO);
    } else if ((index = find(count, arglist, ">>")) != -1) {
        // Open the file in append mode, creating if neccasary with `311`
        // permissions (read / write for user, read for everyone else)
        int fd = open(arglist[index + 1], O_CREAT | O_APPEND | O_WRONLY,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        // Shorten `arglist` to not include `>> filename`
        arglist[index] = NULL;
        redirection(arglist, fd, STDOUT_FILENO);
    } else {
        // Run in foreground
        child1 = fork();
        if (child1 < 0) {
            perror("fork");
            return 1;
        }
        if (child1 == 0) {
            checked_exec(arglist);
        }
    }

    if (child1 > 0) {
        waitpid(child1, NULL, 0);
    }
    if (child2 > 0) {
        waitpid(child2, NULL, 0);
    }
    child1 = -1;
    child2 = -1;
    return 1;
}

int finalize() { return 0; }
