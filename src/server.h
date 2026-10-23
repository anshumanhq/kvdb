#ifndef KVDB_SERVER_H
#define KVDB_SERVER_H

// Server ko start karega. Ye function block karega.
// Port default 6380 (kyunki 6379 pe asli Redis chal raha ho sakta hai).
int server_start(int port);

#endif
