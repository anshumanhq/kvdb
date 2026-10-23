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

#define BACKLOG 10
#define BUF_SIZE 4096

// Client ko data bhejne ka safe tarika
static void send_response(int fd, const char *msg) {
    ssize_t bytes_sent = write(fd, msg, strlen(msg));
    if (bytes_sent < 0) {
        log_error("Failed to send response to client");
    }
}

int server_start(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Database initialize karo
    hashtable *db = ht_create(16);
    if (!db) {
        log_error("Failed to create database");
        return -1;
    }

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

    log_info("Server listening on port %d", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) continue;
        log_info("Client connected!");

        char buffer[BUF_SIZE];
        
        // NAYA LOOP: Ek client ke liye tab tak commands padho jab tak wo disconnect na ho
        while (1) {
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            
            // Agar 0 ya negative aaya, matlab client disconnect ho gaya
            if (bytes_read <= 0) {
                break; 
            }
            
            buffer[bytes_read] = '\0';
            command_t cmd;
            
            int parsed = resp_parse(buffer, bytes_read, &cmd);
            
            if (parsed == -1) {
                send_response(client_fd, "-ERR Protocol error\r\n");
            } else if (parsed > 0) {
                // PING
                if (strcasecmp(cmd.argv[0], "PING") == 0) {
                    send_response(client_fd, "+PONG\r\n");
                } 
                // SET key value
                else if (strcasecmp(cmd.argv[0], "SET") == 0 && cmd.argc == 3) {
                    int rc = ht_set(db, cmd.argv[1], strlen(cmd.argv[1]), cmd.argv[2], strlen(cmd.argv[2]));
                    if (rc >= 0) send_response(client_fd, "+OK\r\n");
                    else send_response(client_fd, "-ERR internal error\r\n");
                }
                // GET key
                else if (strcasecmp(cmd.argv[0], "GET") == 0 && cmd.argc == 2) {
                    size_t val_len = 0;
                    void *val = ht_get(db, cmd.argv[1], strlen(cmd.argv[1]), &val_len);
                    if (val) {
                        char *res = malloc(val_len + 32);
                        snprintf(res, val_len + 32, "$%zu\r\n", val_len);
                        send_response(client_fd, res);
                        
                        // Value bhejo aur return value check karo (-Werror fix)
                        if (write(client_fd, val, val_len) < 0) {
                            log_error("Failed to send value");
                        }
                        
                        send_response(client_fd, "\r\n");
                        free(res);
                    } else {
                        send_response(client_fd, "$-1\r\n"); // Null bulk string
                    }
                }
                // DEL key
                else if (strcasecmp(cmd.argv[0], "DEL") == 0 && cmd.argc == 2) {
                    int rc = ht_del(db, cmd.argv[1], strlen(cmd.argv[1]));
                    if (rc == 1) send_response(client_fd, ":1\r\n");
                    else send_response(client_fd, ":0\r\n");
                }
                else {
                    send_response(client_fd, "-ERR unknown command\r\n");
                }
                command_free(&cmd);
            }
        }
        
        close(client_fd);
        log_info("Client disconnected.");
    }
    ht_free(db);
    return 0;
}
