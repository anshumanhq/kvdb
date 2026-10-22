#include <stdio.h>
#include <string.h> 

#ifndef VERSION
#define VERSION "dev"
#endif
#ifndef BUILD_TIME
#define BUILD_TIME "unknown"
#endif

static void print_usage(const char *prog)
{
    printf("Usage: %s [--help] [--version]\n", prog);
    printf("\n");
    printf("A Redis-like in-memory key-value store.\n");
    printf("\n");
    printf("Options:\n");
    printf("  --help      Show this help and exit\n");
    printf("  --version   Print version and build time\n");
}

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            printf("kvdb %s (built %s)\n", VERSION, BUILD_TIME);
            return 0;
        }
    }
    printf("kvdb %s (built %s)\n", VERSION, BUILD_TIME);
    return 0;
}
