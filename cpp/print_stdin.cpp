#include <stdio.h>
#include <string.h>
#include <memory>


int main() {
    char buffer[128];
    while (true) {
        if (fgets(buffer, 128, stdin) != NULL) {
            if (0 == strcmp(buffer, "stop\n"))
                break;
            else
                fputs(buffer, stdout);
        }
    }

    return 0;
}
