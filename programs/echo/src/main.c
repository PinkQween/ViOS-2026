#include <stdio.h>

int main(int argc, char** argv)
{
        (void)argc;
        (void)argv;
    
        printf("Echo program!\n");
        printf("Arguments:\n");
        for (int i = 0; i < argc; i++) {
            printf("\t%d: %s\n", i, argv[i]);
        }

    return 0;
}
