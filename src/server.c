#include "server.h"
#include "protocol.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BACKLOG 10
#define BUF_SIZE 4096

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
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            command_t cmd;
            
            int parsed = resp_parse(buffer, bytes_read, &cmd);
            
            if (parsed == -1) {
                send_response(client_fd, "-ERR Protocol error\r\n");
            } else if (parsed > 0) {
                if (strcasecmp(cmd.argv[0], "PING") == 0) {
                    send_response(client_fd, "+PONG\r\n");
                } 
                else if (strcasecmp(cmd.argv[0], "ECHO") == 0 && cmd.argc == 2) {
                    char res[1024];
                    snprintf(res, sizeof(res), "$%zu\r\n%s\r\n", strlen(cmd.argv[1]), cmd.argv[1]);
                    send_response(client_fd, res);
                }
                else {
                    send_response(client_fd, "-ERR unknown command\r\n");
                }
                command_free(&cmd);
            }
        }
        close(client_fd);
    }
    return 0;
}
