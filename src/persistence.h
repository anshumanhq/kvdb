#ifndef KVDB_PERSISTENCE_H
#define KVDB_PERSISTENCE_H

#include "hashtable.h"

// AOF file open karta hai (server start hote waqt)
int aof_open(const char *filename);

// AOF file close karta hai
void aof_close(void);

// Ek command ko AOF file mein append karta hai (RESP format mein)
void aof_append(const char *cmd, const char *key, const char *val);

// Server start hote waqt AOF file se data load karta hai
void aof_load(hashtable *db, const char *filename);

#endif
