#include "vios.h"
#include "stdlib.h"

int main(int argc, char** argv)
{
    vios_print("Hello, ViOS!\n");

    void* mem = malloc(128);
    free(mem);

    while (1)
    {
        if (vios_getkey() != 0)
            vios_print("You pressed a key!\n");
    }

    return 0;
}