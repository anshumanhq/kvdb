#include "server.h"
#include "protocol.h"
#include "util.h"
#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define BACKLOG 10
#define MAX_EVENTS 1024
#define BUF_SIZE 4096

// Har client ka apna state rakhne ke liye struct
typedef struct {
    int fd;
    char buffer[BUF_SIZE];
    size_t buf_len; // Kitne bytes abhi tak buffer mein pade hain
} client_t;

static hashtable *db;

static void send_response(int fd, const char *msg) {
    ssize_t bytes_sent = write(fd, msg, strlen(msg));
    if (bytes_sent < 0) {
        log_error("Failed to send response to client");
    }
}

// Client ko non-blocking banane ke liye
static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Ek single command ko process karta hai
static void process_command(client_t *client, command_t *cmd) {
    if (strcasecmp(cmd->argv[0], "PING") == 0) {
        send_response(client->fd, "+PONG\r\n");
    } 
    else if (strcasecmp(cmd->argv[0], "SET") == 0 && cmd->argc == 3) {
        int rc = ht_set(db, cmd->argv[1], strlen(cmd->argv[1]), cmd->argv[2], strlen(cmd->argv[2]));
        if (rc >= 0) send_response(client->fd, "+OK\r\n");
        else send_response(client->fd, "-ERR internal error\r\n");
    }
    else if (strcasecmp(cmd->argv[0], "GET") == 0 && cmd->argc == 2) {
        size_t val_len = 0;
        void *val = ht_get(db, cmd->argv[1], strlen(cmd->argv[1]), &val_len);
        if (val) {
            char *res = malloc(val_len + 32);
            snprintf(res, val_len + 32, "$%zu\r\n", val_len);
            send_response(client->fd, res);
            if (write(client->fd, val, val_len) < 0) log_error("write failed");
            send_response(client->fd, "\r\n");
            free(res);
        } else {
            send_response(client->fd, "$-1\r\n");
        }
    }
    else if (strcasecmp(cmd->argv[0], "DEL") == 0 && cmd->argc == 2) {
        int rc = ht_del(db, cmd->argv[1], strlen(cmd->argv[1]));
        if (rc == 1) send_response(client->fd, ":1\r\n");
        else send_response(client->fd, ":0\r\n");
    }
    else {
        send_response(client->fd, "-ERR unknown command\r\n");
    }
    command_free(cmd);
}

int server_start(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    db = ht_create(16);
    if (!db) return -1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed"); return -1;
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt"); return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed"); return -1;
    }
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen"); return -1;
    }

    // 1. Epoll instance banao
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1"); return -1;
    }

    // 2. Server socket ko epoll mein add karo (taaki naye connections ka pata chale)
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd"); return -1;
    }

    log_info("Server listening on port %d (epoll)", port);

    // 3. Main Event Loop
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait"); break;
        }

        for (int i = 0; i < nfds; i++) {
            // Case A: Naya client connect hua
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                
                set_nonblocking(client_fd);
                
                client_t *client = calloc(1, sizeof(client_t));
                client->fd = client_fd;
                client->buf_len = 0;

                ev.events = EPOLLIN | EPOLLET; // Edge-triggered mode
                ev.data.ptr = client; // Client struct ka pointer store karo
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
                
                log_info("Client connected (fd=%d)", client_fd);
            } 
            // Case B: Kisi existing client se data aaya hai
            else {
                client_t *client = (client_t *)events[i].data.ptr;
                ssize_t bytes_read = read(client->fd, client->buffer + client->buf_len, BUF_SIZE - client->buf_len - 1);
                
                if (bytes_read <= 0) {
                    // Client disconnect ho gaya
                    log_info("Client disconnected (fd=%d)", client->fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->fd, NULL);
                    close(client->fd);
                    free(client);
                } else {
                    client->buf_len += bytes_read;
                    client->buffer[client->buf_len] = '\0';

                    // Ek read mein multiple commands aa sakti hain (Redis pipelining)
                    // Isliye hum loop mein parse karte hain
                    size_t consumed_total = 0;
                    while (consumed_total < client->buf_len) {
                        command_t cmd;
                        int parsed = resp_parse(client->buffer + consumed_total, client->buf_len - consumed_total, &cmd);
                        
                        if (parsed > 0) {
                            process_command(client, &cmd);
                            consumed_total += parsed;
                        } else if (parsed == 0) {
                            break; // Aadha command aaya hai, agle read ka wait karo
                        } else {
                            send_response(client->fd, "-ERR Protocol error\r\n");
                            consumed_total = client->buf_len; // Drop everything and close
                            break;
                        }
                    }
                    
                    // Jo bacha hua data hai (aadha command), usko buffer ke shuru mein shift karo
                    if (consumed_total > 0) {
                        size_t remaining = client->buf_len - consumed_total;
                        memmove(client->buffer, client->buffer + consumed_total, remaining);
                        client->buf_len = remaining;
                    }
                }
            }
        }
    }
    close(server_fd);
    ht_free(db);
    return 0;
}
