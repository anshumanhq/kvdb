#ifndef KVDB_PROTOCOL_H
#define KVDB_PROTOCOL_H

#include <stddef.h>

#define MAX_ARGS 16

typedef struct {
    int argc;
    char *argv[MAX_ARGS];
} command_t;

/* RESP buffer parse karega. 
 * Returns: 
 *  >0 = kitne bytes consume hue
 *   0 = incomplete (aur data chahiye)
 *  -1 = protocol error 
 */
int resp_parse(const char *buf, size_t len, command_t *cmd);

/* Command ki memory free karne ke liye */
void command_free(command_t *cmd);

#endif
