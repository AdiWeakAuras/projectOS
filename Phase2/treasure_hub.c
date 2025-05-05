#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_HUNT_ID_LENGTH 50
#define COMMAND_FILE ".command.tmp"
#define OUTPUT_FILE ".output.tmp"

volatile sig_atomic_t monitor_pid = 0;
volatile sig_atomic_t command_received = 0;

void handle_sigusr1(int sig) {
    command_received = 1;
}

void handle_sigchld(int sig) {
    int status;
    pid_t pid = wait(&status);
    if (pid == monitor_pid) {
        printf("Monitor process terminated with status: %d\n", WEXITSTATUS(status));
        monitor_pid = 0;
    }
}

void start_monitor() {
    if (monitor_pid != 0) {
        printf("Monitor is already running\n");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process (monitor)
        execl("./treasure_monitor", "./treasure_monitor", NULL);
        perror("execl failed");
        exit(1);
    } else {
        // Parent process
        monitor_pid = pid;
        printf("Monitor started with PID: %d\n", pid);
    }
}

void send_command_to_monitor(const char *command) {
    if (monitor_pid == 0) {
        printf("No monitor process running\n");
        return;
    }

    // Write command to file
    int fd = open(COMMAND_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to open command file");
        return;
    }
    write(fd, command, strlen(command));
    close(fd);

    // Send signal to monitor
    kill(monitor_pid, SIGUSR1);

    // Wait for output (simplified - in real implementation would use proper synchronization)
    sleep(1);

    // Read and display output
    char buffer[1024];
    fd = open(OUTPUT_FILE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open output file");
        return;
    }
    ssize_t bytes_read;
    while (bytes_read = read(fd, buffer, sizeof(buffer))) {
        write(STDOUT_FILENO, buffer, bytes_read);
    }
    close(fd);
}

void stop_monitor() {
    if (monitor_pid == 0) {
        printf("No monitor process running\n");
        return;
    }
    kill(monitor_pid, SIGTERM);
    printf("Sent termination signal to monitor\n");
}

int main() {
    struct sigaction sa_usr1, sa_chld;

    // Set up SIGUSR1 handler
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    // Set up SIGCHLD handler
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = 0;
    sigaction(SIGCHLD, &sa_chld, NULL);

    printf("Treasure Hub - Interactive Interface\n");
    printf("Available commands:\n");
    printf("  start_monitor\n");
    printf("  list_hunts\n");
    printf("  list_treasures <hunt_id>\n");
    printf("  view_treasure <hunt_id> <treasure_id>\n");
    printf("  stop_monitor\n");
    printf("  exit\n");

    char input[MAX_COMMAND_LENGTH];
    while (1) {
        printf("> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        // Remove newline
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        } else if (strncmp(input, "list_hunts", 10) == 0) {
            send_command_to_monitor(input);
        } else if (strncmp(input, "list_treasures", 14) == 0) {
            send_command_to_monitor(input);
        } else if (strncmp(input, "view_treasure", 13) == 0) {
            send_command_to_monitor(input);
        } else if (strcmp(input, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_pid != 0) {
                printf("Error: Monitor process is still running\n");
            } else {
                break;
            }
        } else {
            printf("Unknown command\n");
        }
    }

    return 0;
}