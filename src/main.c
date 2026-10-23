#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"
#include "util.h"

#ifndef VERSION
#define VERSION "dev"
#endif
#ifndef BUILD_TIME
#define BUILD_TIME "unknown"
#endif

static void print_usage(const char *prog)
{
    printf("Usage: %s [--port <port>] [--version]\n", prog);
    printf("\n");
    printf("A Redis-like in-memory key-value store.\n");
    printf("\n");
    printf("Options:\n");
    printf("  --help      Show this help and exit\n");
    printf("  --version   Print version and build time\n");
    printf("  --port <n>  Set listening port (default: 6380)\n");
}

int main(int argc, char **argv)
{
    int port = 6380;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]); // Ab ye use ho gaya, warning nahi aayegi
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            printf("kvdb %s (built %s)\n", VERSION, BUILD_TIME);
            return 0;
        }
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        }
    }

    log_set_level(LOG_DEBUG);
    log_info("Starting kvdb on port %d", port);
    
    // Server start karo
    server_start(port);

    return 0;
}
