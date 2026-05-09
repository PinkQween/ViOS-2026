#include "vios.h"

extern int main(int argc, char** argv);

void c_start()
{
    struct process_arguments args;
    vios_process_get_arguments(&args);
    vios_exit(main(args.argc, args.argv));
}