#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_HUNT_ID_LENGTH 50
#define COMMAND_FILE ".command.tmp"
#define OUTPUT_FILE ".output.tmp"
#define HUNT_DIR_PREFIX "hunt_"
#define TREASURE_FILE "treasures.dat"

typedef struct {
    int treasureID;
    char userName[20];
    double latitude;
    double longitude;
    char clueText[50];
    int value;
} Treasure;

typedef struct {
    char huntID[50];
    Treasure treasures[100];
    int count;
} Hunt;

volatile sig_atomic_t running = 1;

void handle_sigusr1(int sig) {
    // Signal received - check for new command
}

void handle_sigterm(int sig) {
    running = 0;
}

void loadHuntFromFile(Hunt *hunt, const char *huntID) {
    char dirName[100];
    snprintf(dirName, sizeof(dirName), "%s%s", HUNT_DIR_PREFIX, huntID);

    char treasurePath[150];
    snprintf(treasurePath, sizeof(treasurePath), "%s/%s", dirName, TREASURE_FILE);

    int fd = open(treasurePath, O_RDONLY);
    if (fd == -1) {
        hunt->count = 0;
        return;
    }

    read(fd, &hunt->count, sizeof(int));
    for (int i = 0; i < hunt->count; i++) {
        read(fd, &hunt->treasures[i], sizeof(Treasure));
    }
    close(fd);
}

void listHunts() {
    DIR *dir;
    struct dirent *ent;
    int output_fd = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, HUNT_DIR_PREFIX, strlen(HUNT_DIR_PREFIX)) == 0) {
                Hunt hunt = {0};
                char huntID[50];
                strncpy(huntID, ent->d_name + strlen(HUNT_DIR_PREFIX), sizeof(huntID));
                loadHuntFromFile(&hunt, huntID);
                dprintf(output_fd, "Hunt: %s (%d treasures)\n", huntID, hunt.count);
            }
        }
        closedir(dir);
    }
    close(output_fd);
}

void listTreasures(const char *huntID) {
    Hunt hunt = {0};
    loadHuntFromFile(&hunt, huntID);
    int output_fd = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    dprintf(output_fd, "Treasures in hunt %s:\n", huntID);
    for (int i = 0; i < hunt.count; i++) {
        Treasure *t = &hunt.treasures[i];
        dprintf(output_fd, "%d: %s (%.6f, %.6f) - %s (Value: %d)\n",
               t->treasureID, t->userName, t->latitude, t->longitude, t->clueText, t->value);
    }
    close(output_fd);
}

void viewTreasure(const char *huntID, int treasureID) {
    Hunt hunt = {0};
    loadHuntFromFile(&hunt, huntID);
    int output_fd = open(OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    for (int i = 0; i < hunt.count; i++) {
        if (hunt.treasures[i].treasureID == treasureID) {
            Treasure *t = &hunt.treasures[i];
            dprintf(output_fd, "Treasure ID: %d\n", t->treasureID);
            dprintf(output_fd, "User: %s\n", t->userName);
            dprintf(output_fd, "Coordinates: (%.6f, %.6f)\n", t->latitude, t->longitude);
            dprintf(output_fd, "Clue: %s\n", t->clueText);
            dprintf(output_fd, "Value: %d\n", t->value);
            close(output_fd);
            return;
        }
    }
    dprintf(output_fd, "Treasure with ID %d not found in hunt %s\n", treasureID, huntID);
    close(output_fd);
}

void process_command(const char *command) {
    char cmd[20], huntID[50];
    int treasureID;

    if (sscanf(command, "list_hunts") == 1) {
        listHunts();
    } else if (sscanf(command, "list_treasures %s", huntID) == 1) {
        listTreasures(huntID);
    } else if (sscanf(command, "view_treasure %s %d", huntID, &treasureID) == 2) {
        viewTreasure(huntID, treasureID);
    }
}

int main() {
    struct sigaction sa_usr1, sa_term;

    // Set up signal handlers
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    sa_term.sa_handler = handle_sigterm;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    printf("Monitor process started (PID: %d)\n", getpid());

    while (running) {
        // Check for new commands
        char command[MAX_COMMAND_LENGTH];
        int fd = open(COMMAND_FILE, O_RDONLY);
        if (fd != -1) {
            ssize_t bytes_read = read(fd, command, sizeof(command));
            close(fd);
            if (bytes_read > 0) {
                command[bytes_read] = '\0';
                process_command(command);
            }
        }
        usleep(100000); // Sleep for 100ms
    }

    printf("Monitor process terminating\n");
    return 0;
}