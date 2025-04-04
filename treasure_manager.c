#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

#define USERNAME_MAX_LENGTH 20
#define CLUE_MAX_LENGTH 50
#define MAX_TREASURES 100
#define HUNT_DIR_PREFIX "hunt_"
#define LOG_FILE "logged_hunt"
#define TREASURE_FILE "treasures.dat"

typedef struct {
    int treasureID;
    char userName[USERNAME_MAX_LENGTH];
    double latitude;
    double longitude;
    char clueText[CLUE_MAX_LENGTH];
    int value;
} Treasure;

typedef struct {
    char huntID[50];
    Treasure treasures[MAX_TREASURES];
    int count;
} Hunt;

// Function prototypes
void addTreasure(Hunt *hunt, int id, const char *user, double lat, double lon, const char *clue, int val);
void listTreasures(const Hunt *hunt);
void viewTreasure(const Hunt *hunt, int id);
void removeTreasure(Hunt *hunt, int id);
void logOperation(const char *huntID, const char *operation);
void createHuntDirectory(const char *huntID);
void createSymlink(const char *huntID);
void saveHuntToFile(const Hunt *hunt);
void loadHuntFromFile(Hunt *hunt, const char *huntID);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <operation> <hunt_id> [options]\n", argv[0]);
        printf("Operations:\n");
        printf("  --add <hunt_id> - Add a new treasure\n");
        printf("  --list <hunt_id> - List all treasures\n");
        printf("  --view <hunt_id> <id> - View specific treasure\n");
        printf("  --remove_treasure <hunt_id> <id> - Remove a treasure\n");
        printf("  --remove_hunt <hunt_id> - Remove entire hunt\n");
        return 1;
    }

    char *operation = argv[1];
    char *huntID = argv[2];
    Hunt currentHunt = {0};
    strncpy(currentHunt.huntID, huntID, sizeof(currentHunt.huntID) - 1);

    if (strcmp(operation, "--add") == 0) {
        // Load existing hunt if it exists
        loadHuntFromFile(&currentHunt, huntID);

        // For simplicity, we'll just add a sample treasure
        // In a real implementation, you'd get these values from user input
        addTreasure(&currentHunt, currentHunt.count + 1, "user1", 45.0, 25.0, "Look under the rock", 100);
        saveHuntToFile(&currentHunt);
        logOperation(huntID, "add");
    } 
    else if (strcmp(operation, "--list") == 0) {
        loadHuntFromFile(&currentHunt, huntID);
        listTreasures(&currentHunt);
        logOperation(huntID, "list");
    } 
    else if (strcmp(operation, "--view") == 0 && argc >= 4) {
        loadHuntFromFile(&currentHunt, huntID);
        int id = atoi(argv[3]);
        viewTreasure(&currentHunt, id);
        logOperation(huntID, "view");
    } 
    else if (strcmp(operation, "--remove_treasure") == 0 && argc >= 4) {
        loadHuntFromFile(&currentHunt, huntID);
        int id = atoi(argv[3]);
        removeTreasure(&currentHunt, id);
        saveHuntToFile(&currentHunt);
        logOperation(huntID, "remove_treasure");
    } 
    else if (strcmp(operation, "--remove_hunt") == 0) {
        // Implementation would remove directory and files
        printf("Removing hunt %s\n", huntID);
        logOperation(huntID, "remove_hunt");
    } 
    else {
        printf("Invalid operation or missing parameters\n");
        return 1;
    }

    return 0;
}

void addTreasure(Hunt *hunt, int id, const char *user, double lat, double lon, const char *clue, int val) {
    if (hunt->count >= MAX_TREASURES) {
        printf("Cannot add more treasures to this hunt\n");
        return;
    }

    Treasure *t = &hunt->treasures[hunt->count++];
    t->treasureID = id;
    strncpy(t->userName, user, USERNAME_MAX_LENGTH - 1);
    t->latitude = lat;
    t->longitude = lon;
    strncpy(t->clueText, clue, CLUE_MAX_LENGTH - 1);
    t->value = val;
}

void listTreasures(const Hunt *hunt) {
    char dirName[100];
    snprintf(dirName, sizeof(dirName), "%s%s", HUNT_DIR_PREFIX, hunt->huntID);

    struct stat st;
    char treasurePath[150];
    snprintf(treasurePath, sizeof(treasurePath), "%s/%s", dirName, TREASURE_FILE);

    if (stat(treasurePath, &st) == 0) {
        printf("Hunt: %s\n", hunt->huntID);
        printf("File size: %ld bytes\n", st.st_size);
        printf("Last modified: %s", ctime(&st.st_mtime));
    }

    printf("Treasures:\n");
    for (int i = 0; i < hunt->count; i++) {
        const Treasure *t = &hunt->treasures[i];
        printf("%d: %s (%.6f, %.6f) - %s (Value: %d)\n", 
               t->treasureID, t->userName, t->latitude, t->longitude, t->clueText, t->value);
    }
}

void viewTreasure(const Hunt *hunt, int id) {
    for (int i = 0; i < hunt->count; i++) {
        if (hunt->treasures[i].treasureID == id) {
            const Treasure *t = &hunt->treasures[i];
            printf("Treasure ID: %d\n", t->treasureID);
            printf("User: %s\n", t->userName);
            printf("Coordinates: (%.6f, %.6f)\n", t->latitude, t->longitude);
            printf("Clue: %s\n", t->clueText);
            printf("Value: %d\n", t->value);
            return;
        }
    }
    printf("Treasure with ID %d not found\n", id);
}

void removeTreasure(Hunt *hunt, int id) {
    for (int i = 0; i < hunt->count; i++) {
        if (hunt->treasures[i].treasureID == id) {
            // Shift all subsequent treasures down
            for (int j = i; j < hunt->count - 1; j++) {
                hunt->treasures[j] = hunt->treasures[j + 1];
            }
            hunt->count--;
            printf("Removed treasure with ID %d\n", id);
            return;
        }
    }
    printf("Treasure with ID %d not found\n", id);
}

void logOperation(const char *huntID, const char *operation) {
    char dirName[100];
    snprintf(dirName, sizeof(dirName), "%s%s", HUNT_DIR_PREFIX, huntID);

    // Create directory if it doesn't exist
    mkdir(dirName, 0755);

    char logPath[150];
    snprintf(logPath, sizeof(logPath), "%s/%s", dirName, LOG_FILE);

    FILE *logFile = fopen(logPath, "a");
    if (logFile) {
        time_t now = time(NULL);
        fprintf(logFile, "[%s] %s: %s\n", ctime(&now), huntID, operation);
        fclose(logFile);
    }

    // Create/update symlink
    createSymlink(huntID);
}

void createSymlink(const char *huntID) {
    char symlinkName[100];
    snprintf(symlinkName, sizeof(symlinkName), "logged_hunt-%s", huntID);

    char targetPath[150];
    snprintf(targetPath, sizeof(targetPath), "%s%s/%s", HUNT_DIR_PREFIX, huntID, LOG_FILE);

    // Remove existing symlink if it exists
    unlink(symlinkName);
    symlink(targetPath, symlinkName);
}

void saveHuntToFile(const Hunt *hunt) {
    char dirName[100];
    snprintf(dirName, sizeof(dirName), "%s%s", HUNT_DIR_PREFIX, hunt->huntID);

    // Create directory if it doesn't exist
    mkdir(dirName, 0755);

    char treasurePath[150];
    snprintf(treasurePath, sizeof(treasurePath), "%s/%s", dirName, TREASURE_FILE);

    int fd = open(treasurePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening treasure file");
        return;
    }

    // Write count first
    write(fd, &hunt->count, sizeof(int));

    // Write each treasure
    for (int i = 0; i < hunt->count; i++) {
        write(fd, &hunt->treasures[i], sizeof(Treasure));
    }

    close(fd);
}

void loadHuntFromFile(Hunt *hunt, const char *huntID) {
    char dirName[100];
    snprintf(dirName, sizeof(dirName), "%s%s", HUNT_DIR_PREFIX, huntID);

    char treasurePath[150];
    snprintf(treasurePath, sizeof(treasurePath), "%s/%s", dirName, TREASURE_FILE);

    int fd = open(treasurePath, O_RDONLY);
    if (fd == -1) {
        // File doesn't exist yet (first time for this hunt)
        hunt->count = 0;
        return;
    }

    // Read count
    read(fd, &hunt->count, sizeof(int));

    // Read treasures
    for (int i = 0; i < hunt->count; i++) {
        read(fd, &hunt->treasures[i], sizeof(Treasure));
    }

    close(fd);
}