#include "persistence.h"
#include "protocol.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

static int aof_fd = -1;

int aof_open(const char *filename) {
    aof_fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (aof_fd == -1) {
        log_error("Failed to open AOF file: %s", filename);
        return -1;
    }
    log_info("AOF file opened: %s", filename);
    return 0;
}

void aof_close(void) {
    if (aof_fd != -1) {
        fsync(aof_fd);
        close(aof_fd);
        aof_fd = -1;
    }
}

void aof_append(const char *cmd, const char *key, const char *val) {
    if (aof_fd == -1) return;

    size_t max_len = strlen(cmd) + strlen(key) + (val ? strlen(val) : 0) + 100;
    char *buffer = malloc(max_len); // Dynamic allocation to prevent stack overflow
    if (!buffer) return;

    int len = 0;
    if (val) {
        len = snprintf(buffer, max_len, "*3\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n",
            strlen(cmd), cmd, strlen(key), key, strlen(val), val);
    } else {
        len = snprintf(buffer, max_len, "*2\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n",
            strlen(cmd), cmd, strlen(key), key);
    }

    if (len > 0 && len < (int)max_len) {
        ssize_t written = write(aof_fd, buffer, len);
        if (written != len) {
            log_error("Failed to write to AOF file");
        }
        fsync(aof_fd); // Fsync for durability
    }
    free(buffer);
}

static void process_loaded_command(hashtable *db, command_t *cmd) {
    if (strcasecmp(cmd->argv[0], "SET") == 0 && cmd->argc == 3) {
        ht_set(db, cmd->argv[1], strlen(cmd->argv[1]), cmd->argv[2], strlen(cmd->argv[2]));
    } else if (strcasecmp(cmd->argv[0], "DEL") == 0 && cmd->argc == 2) {
        ht_del(db, cmd->argv[1], strlen(cmd->argv[1]));
    } else if (strcasecmp(cmd->argv[0], "EXPIRE") == 0 && cmd->argc == 3) {
        int sec = atoi(cmd->argv[2]);
        if (sec > 0) {
            ht_set_expire(db, cmd->argv[1], strlen(cmd->argv[1]), time(NULL) + sec);
        }
    }
}

void aof_load(hashtable *db, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        log_info("No AOF file found, starting fresh.");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size <= 0) { fclose(fp); return; }

    char *buf = malloc(file_size);
    if (!buf) { fclose(fp); return; }

    if (fread(buf, 1, file_size, fp) != (size_t)file_size) {
        free(buf); fclose(fp); return;
    }
    
    size_t consumed = 0;
    int loaded_count = 0;
    
    while (consumed < (size_t)file_size) {
        command_t cmd;
        int parsed = resp_parse(buf + consumed, file_size - consumed, &cmd);
        
        if (parsed > 0) {
            if (cmd.argc > 0) {
                process_loaded_command(db, &cmd);
            }
            command_free(&cmd);
            consumed += parsed;
            loaded_count++;
        } else {
            break;
        }
    }
    
    log_info("AOF loaded successfully. %d commands executed.", loaded_count);
    free(buf);
    fclose(fp);
}