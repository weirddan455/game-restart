#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

static void start_game(char **arguments)
{
    printf("Executing \"");
    for (int i = 0; arguments[i] != NULL; i++) {
        if (i == 0) {
            printf("%s", arguments[i]);
        } else {
            printf(" %s", arguments[i]);
        }
    }
    printf("\"\n");
    int pid = fork();
    switch(pid) {
        case -1:
            perror("fork failed");
            break;
        case 0: {
            struct sigaction action = { 0 };
            action.sa_handler = SIG_IGN;
            sigaction(SIGHUP, &action, NULL);
            sigaction(SIGINT, &action, NULL);
            int old_stderr = fcntl(STDERR_FILENO, F_DUPFD_CLOEXEC, 3);
            int null_fd = open("/dev/null", O_RDWR | O_CLOEXEC);
            dup2(null_fd, STDIN_FILENO);
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
            execvp(arguments[0], arguments);
            dup2(old_stderr, STDERR_FILENO);
            perror("Failed to start program");
            exit(EXIT_FAILURE);
        }
    }
}

static int get_crash_pid(char *buffer)
{
    char *ptr = strstr(buffer, "NVRM: Xid");
    if (ptr == NULL) {
        return 0;
    }
    ptr = strstr(ptr, "pid=");
    if (ptr == NULL) {
        return 0;
    }
    return atoi(ptr + 4);
}

int main(int argc, char **argv)
{
    struct sigaction action = { 0 };
    action.sa_handler = SIG_DFL;
    action.sa_flags = SA_NOCLDWAIT;
    if (sigaction(SIGCHLD, &action, NULL) == -1) {
        perror("sigaction failed");
        return EXIT_FAILURE;
    }
    char **game_arguments = NULL;
    if (argc > 1) {
        game_arguments = malloc(argc * sizeof(char *));
        if (game_arguments == NULL) {
            fprintf(stderr, "malloc failed\n");
            return EXIT_FAILURE;
        }
        for (int i = 0; i < argc; i++) {
            game_arguments[i] = argv[i + 1];
        }
    }
    char *buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "malloc failed\n");
        return EXIT_FAILURE;
    }
    int fd = open("/dev/kmsg", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        perror("failed to open /dev/kmsg");
        return EXIT_FAILURE;
    }
    if (lseek(fd, 0, SEEK_END) == -1) {
        perror("lseek failed");
        return EXIT_FAILURE;
    }
    if (game_arguments != NULL) {
        start_game(game_arguments);
    }
    while (1) {
        ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("read from /dev/kmsg failed");
            return EXIT_FAILURE;
        } else if (bytes_read < 1) {
            fprintf(stderr, "Invalid number of bytes read from /dev/kmsg: %zd\n", bytes_read);
            return EXIT_FAILURE;
        }
        buffer[bytes_read] = '\0';
        int pid = get_crash_pid(buffer);
        if (pid > 0) {
            printf("Xid detected for PID %d. Sending SIGTERM...\n", pid);
            kill(pid, SIGTERM);
            if (game_arguments != NULL) {
                sleep(5);
                start_game(game_arguments);
            }
        }
    }
    return EXIT_SUCCESS;
}
