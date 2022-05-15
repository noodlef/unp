#include <stdio.h>
#include <unistd.h>

#include "../../lib/lib.h"


int main(int argc, char ** argv, char ** environ)
{
    int i;

    printf("==================================\n");
    for (i = 0; environ[i]; i++) {
        printf("%s\n", environ[i]);
    }

    printf("\n\n");

    set_proctitile(argv, "server_program");
    set_proctitile(argv, "noodles-test-server");

    printf("==================================\n");
    for (i = 0; environ[i]; i++) {
        printf("%s\n", environ[i]);
    }

    sleep(600);

    return 0;
}
