#include "bobbin-internal.h"

#include <stddef.h> // NULL

extern void do_config(int, char **);

const char *program_name;

int main(int argc, char **argv)
{
    program_name = *argv;
    do_config(argc, argv);

    bobbin_run();

    return 0;
}
