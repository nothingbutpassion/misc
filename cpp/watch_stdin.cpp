#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

bool readStdin() {
    char buf[256] = { 0 };
    int numRead = read(0, buf, sizeof(buf)-1);
    if (numRead == -1) {
        perror("read fail");
        return false;
    }

    if (numRead == 0) {
        printf("read 0 bytes.\n");
        return false;
    }

    printf("read %d bytes: \"%s\"\n", numRead, buf);
    return true;
}

int main(int argc, char* argv[])
{
    int block = fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    printf("in %s mode\n", block ? "block" : "unblock");
    
    while (true) {
        if (!readStdin()) {
            printf("will sleep 1s and try again\n");
            usleep(1000000);     
        }
    }

    return 0;
}
