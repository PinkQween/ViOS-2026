#include "stdio.h"
#include "vios.h"
#include "stdbool.h"

int main(int argc, char** argv)
{
    printf("ViOS v0.1.0\n");

    while (1) {
        printf("> ");
        char buffer[2048];
        vios_terminal_readline(buffer, sizeof(buffer), true);
        putchar('\n');
        vios_system_run(buffer);
    }

    return 0;
}