#include "vios.h"
#include "stdbool.h"

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    vios_print("ViOS v0.1.0\n");

    while (1) {
        vios_print("> ");
        char buffer[2048];
        vios_terminal_readline(buffer, sizeof(buffer), true);
        vios_print("\n");
        vios_system_run(buffer);
    }

    return 0;
}