#include "persistence.h"
#include "protocol.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>

static int aof_fd = -1;

int aof_open(const char *filename) {
    // O_APPEND: File ke end mein add karo
    // O_CREAT: Agar file nahi hai toh bana do
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
        close(aof_fd);
        aof_fd = -1;
    }
}

void aof_append(const char *cmd, const char *key, const char *val) {
    if (aof_fd == -1) return;

    char buffer[4096];
    int len = 0;
    
    if (val) {
        // SET command format: *3\r\n$3\r\nSET\r\n$len\r\nkey\r\n$len\r\nval\r\n
        len = snprintf(buffer, sizeof(buffer), 
            "*3\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n",
            strlen(cmd), cmd, strlen(key), key, strlen(val), val);
    } else {
        // DEL command format: *2\r\n$3\r\nDEL\r\n$len\r\nkey\r\n
        len = snprintf(buffer, sizeof(buffer), 
            "*2\r\n$%zu\r\n%s\r\n$%zu\r\n%s\r\n",
            strlen(cmd), cmd, strlen(key), key);
    }

    if (len > 0) {
        ssize_t written = write(aof_fd, buffer, len);
        if (written != len) {
            log_error("Failed to write to AOF file");
        }
    }
}

// AOF load karte waqt commands ko database mein apply karna
static void process_loaded_command(hashtable *db, command_t *cmd) {
    if (strcasecmp(cmd->argv[0], "SET") == 0 && cmd->argc == 3) {
        ht_set(db, cmd->argv[1], strlen(cmd->argv[1]), cmd->argv[2], strlen(cmd->argv[2]));
    } else if (strcasecmp(cmd->argv[0], "DEL") == 0 && cmd->argc == 2) {
        ht_del(db, cmd->argv[1], strlen(cmd->argv[1]));
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
    
    if (file_size <= 0) { 
        fclose(fp); 
        return; 
    }

    char *buf = malloc(file_size);
    if (!buf) { 
        fclose(fp); 
        return; 
    }

    if (fread(buf, 1, file_size, fp) != (size_t)file_size) {
        free(buf); 
        fclose(fp); 
        return;
    }
    
    size_t consumed = 0;
    int loaded_count = 0;
    
    while (consumed < (size_t)file_size) {
        command_t cmd;
        int parsed = resp_parse(buf + consumed, file_size - consumed, &cmd);
        
        if (parsed > 0) {
            process_loaded_command(db, &cmd);
            command_free(&cmd);
            consumed += parsed;
            loaded_count++;
        } else {
            // Incomplete ya corrupted data
            break; 
        }
    }
    
    log_info("AOF loaded successfully. %d commands executed.", loaded_count);
    free(buf);
    fclose(fp);
}
