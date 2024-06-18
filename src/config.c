#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {//save the configuration
    char* directory;
    int max_peers;
    uint16_t port;
} Config;

Config* load_config(const char* path) {
    Config* config = malloc(sizeof(Config));
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return NULL;
    }

    char *file_in_memory = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_in_memory == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    // Parse the file in memory
    char *cfg = file_in_memory;

    // Parse the directory whatever the length
    char* start = strstr(cfg, "directory:");
    if (start == NULL) {
        close(fd);
        return NULL;
    }
    start += strlen("directory:");  // Move the pointer to the start of the directory

    char* end = strchr(start, '\n');
    if (end == NULL) {
        close(fd);
        return NULL;
    }

    size_t length = end - start;
    config->directory = malloc(length + 1);
    if (config->directory == NULL) {
        close(fd);
        return NULL;
    }

    memcpy(config->directory, start, length);
    config->directory[length] = '\0';  

    cfg = end + 1;


    if (stat(config->directory, &st) == -1) {
        // Directory does not exist, try to create it
        if (mkdir(config->directory, 0700) == -1) {
            perror("Unable to create directory");
            exit(3);
        }
    } else if (S_ISREG(st.st_mode)) {
        // Directory is a file
        fprintf(stderr, "Directory is a file.\n");
        exit(3);
    }

    sscanf(cfg, "max_peers:%u\n", &config->max_peers);
    cfg = strchr(cfg, '\n') + 1;
    // Check if max_peers is within the valid range
    if (config->max_peers < 1 || config->max_peers > 2048) {
        exit(4);
    }

    sscanf(cfg, "port:%hu\n", &config->port);
    cfg = strchr(cfg, '\n') + 1;
    if (config->port <= 1024 || config->port > 65535) {
    exit(5);
    }
    

    munmap(file_in_memory, st.st_size);
    close(fd);

    return config;
}
// Free the configuration
void free_config(Config* config) {
    if (config) {
        if (config->directory) {
            free(config->directory);
        }
        free(config);
    }
}