#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper: \r\n dhoondhne ke liye
static const char* find_crlf(const char *buf, size_t len) {
    for (size_t i = 0; i < len - 1; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') return buf + i;
    }
    return NULL;
}

int resp_parse(const char *buf, size_t len, command_t *cmd) {
    cmd->argc = 0;
    if (len == 0) return 0;

    // Phase 1: Sirf Array (*) type support kar rahe hain abhi
    if (buf[0] != '*') return -1; 

    const char *crlf = find_crlf(buf, len);
    if (!crlf) return 0; // Incomplete

    // Parse number of arguments
    int num_args = atoi(buf + 1);
    if (num_args <= 0 || num_args > MAX_ARGS) return -1;

    size_t consumed = (crlf - buf) + 2;
    const char *ptr = crlf + 2;

    for (int i = 0; i < num_args; i++) {
        // Har argument $ se shuru hona chahiye (Bulk String)
        if (consumed >= len || *ptr != '$') return -1;
        
        crlf = find_crlf(ptr, len - consumed);
        if (!crlf) return 0; // Incomplete

        int arg_len = atoi(ptr + 1);
        if (arg_len < 0) return -1;

        consumed += (crlf - ptr) + 2;
        ptr = crlf + 2;

        // Check agar pura string aa chuka hai
        if (consumed + arg_len + 2 > len) return 0; // Incomplete

        // Argument copy karo
        cmd->argv[i] = malloc(arg_len + 1);
        if (!cmd->argv[i]) return -1;
        memcpy(cmd->argv[i], ptr, arg_len);
        cmd->argv[i][arg_len] = '\0'; // Null-terminate for C strings

        consumed += arg_len + 2;
        ptr += arg_len + 2;
        cmd->argc++;
    }

    return consumed;
}

void command_free(command_t *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }
    cmd->argc = 0;
}
