#include <stdio.h>

int main(int argc, char** argv, char** env) {

    char** ap;
    puts("Arguments:");
    for (ap = argv; *ap != NULL; ap++)
        puts(*ap);    

    puts("Environments:");
    char **ep;
    for (ep = env; *ep != NULL; ep++)
        puts(*ep);

    return 0;
}
