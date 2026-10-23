#include "protocol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

static const char* find_crlf(const char *buf, size_t len) {
    for (size_t i = 0; i < len - 1; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') return buf + i;
    }
    return NULL;
}

void command_free(command_t *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }
    cmd->argc = 0;
}

int resp_parse(const char *buf, size_t len, command_t *cmd) {
    cmd->argc = 0;
    if (len == 0) return 0;

    // Inline Commands
    if (buf[0] != '*') {
        const char *crlf = find_crlf(buf, len);
        if (!crlf) return 0; // Incomplete

        size_t line_len = crlf - buf;
        if (line_len == 0) return (crlf - buf) + 2; // Empty line, just consume it

        char *temp = malloc(line_len + 1);
        if (!temp) return -1;
        memcpy(temp, buf, line_len);
        temp[line_len] = '\0';

        char *token = strtok(temp, " ");
        while (token != NULL && cmd->argc < MAX_ARGS) {
            cmd->argv[cmd->argc] = strdup(token);
            cmd->argc++;
            token = strtok(NULL, " ");
        }
        free(temp);
        return (crlf - buf) + 2;
    }

    // RESP Array format
    const char *crlf = find_crlf(buf, len);
    if (!crlf) return 0;

    int num_args = atoi(buf + 1);
    if (num_args <= 0 || num_args > MAX_ARGS) return -1;

    size_t consumed = (crlf - buf) + 2;
    const char *ptr = crlf + 2;

    for (int i = 0; i < num_args; i++) {
        if (consumed >= len || *ptr != '$') {
            command_free(cmd); // LEAK FIX
            return -1;
        }
        
        crlf = find_crlf(ptr, len - consumed);
        if (!crlf) {
            command_free(cmd); // LEAK FIX
            return 0;
        }

        int arg_len = atoi(ptr + 1);
        if (arg_len < 0) {
            command_free(cmd);
            return -1;
        }

        consumed += (crlf - ptr) + 2;
        ptr = crlf + 2;

        if (consumed + arg_len + 2 > len) {
            command_free(cmd); // LEAK FIX
            return 0;
        }

        cmd->argv[i] = malloc(arg_len + 1);
        if (!cmd->argv[i]) {
            command_free(cmd);
            return -1;
        }
        memcpy(cmd->argv[i], ptr, arg_len);
        cmd->argv[i][arg_len] = '\0';

        consumed += arg_len + 2;
        ptr += arg_len + 2;
        cmd->argc++;
    }

    return consumed;
}