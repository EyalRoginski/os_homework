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
        fprintf(stderr, "Fatal error: %s\n", strerror(errno));
    }
    return 0;
}

/**
 * Run the program as dictated by arglist, with the child's STDIN pointed to
 * `stdin`, and its STDOUT to `stdout`. Does not wait for it to exit, returns
 * the pid of the child process.
 * */
int run(char **arglist, int new_stdin, int new_stdout) {
    int child_pid = fork();
    if (child_pid < 0) {
        fprintf(stderr, "Fatal error: %s\n", strerror(errno));
    }
    if (child_pid == 0) {
        int dup_1 = dup2(new_stdin, STDIN_FILENO);
        int dup_2 = dup2(new_stdout, STDOUT_FILENO);
        if (dup_1 == -1 || dup_2 == -1) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
        }
        if (execvp(arglist[0], arglist) == -1) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(1);
        }
        // execvp should not return
    }
    return child_pid;
}

/**
 * Like `run`, but the child's STDIN and STDOUT are not changed.
 */
int run_std(char **arglist) {
    return run(arglist, STDIN_FILENO, STDOUT_FILENO);
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

int process_arglist(int count, char **arglist) {
    if (strcmp(arglist[0], "exit") == 0) {
        return 0;
    }
    int index;
    int wait_for_children = 1;
    if (strcmp(arglist[count - 1], "&") == 0) {
        // Run in background
        arglist[count - 1] = NULL;
        run_std(arglist);
        wait_for_children = 0;
    } else if ((index = find(count, arglist, "|")) != -1) {
        // Open a pipe, for communication between the piped processes
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            fprintf(stderr, "Fatal error: %s\n", strerror(errno));
        }

        arglist[index] = NULL;
        child1 = run(arglist, STDIN_FILENO, pipefd[1]);
        child2 = run(arglist + index + 1, pipefd[0], STDOUT_FILENO);
    } else if ((index = find(count, arglist, "<")) != -1) {
        int fd = open(arglist[index + 1], 0);
        if (fd == -1) {
            return 0;
        }

        child1 = run(arglist, fd, STDOUT_FILENO);
    } else if ((index = find(count, arglist, ">>")) != -1) {
        // Open the file in append mode, creating if neccasary with `311`
        // permissions (read / write for user, read for everyone else)
        int fd = open(arglist[index + 1], O_CREAT | O_APPEND | O_WRONLY,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        // Shorten `arglist` to not include `>> filename`
        arglist[index] = NULL;
        child1 = run(arglist, STDIN_FILENO, fd);
    } else {
        // Run in foreground
        child1 = run_std(arglist);
    }

    if (wait_for_children) {
        if (child1 > 0) {
            waitpid(child1, NULL, 0);
        }
        if (child2 > 0) {
            waitpid(child2, NULL, 0);
        }
    }
    child1 = -1;
    child2 = -1;
    return 1;
}

int finalize() { return 0; }
