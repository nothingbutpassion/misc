#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sstream>

using namespace std;

void usage(const char* appPath) {
    fprintf(stderr, "usage: %s <prog> <args...> [ --setenvs <envs...> ]\n", appPath);
    fprintf(stderr, "example: %s /bin/sh  sh -c \"ps -ef\" [ --setenvs X=100 Y=100 ]\n", appPath);
}

int main(int argc, char** argv) {
    
    if (argc < 3) {
        usage(argv[0]);
        return -1;    
    }

    char* args[256] = { 0 };
    int   argi = 0;
    char* envs[256] = { 0 };
    int   envi = 0;
    bool hasEnv = false;
    for (int i=2; i < argc; i++) {
        if (string(argv[i]) == "--setenvs") {
            hasEnv = true;
            continue;     
        }

        if (hasEnv) {
            envs[envi] = new char[strlen(argv[i]) + 1];
            strcpy(envs[envi], argv[i]);
            envi++;
        } else {
            args[argi] = new char[strlen(argv[i]) + 1];
            strcpy(args[argi], argv[i]);
            argi++;
        }
    }

    if (argi == 0) {
        usage(argv[0]);
        return -1;
    }

    if (!hasEnv) {
        if (-1 == execv(argv[1], args)) {
            fprintf(stderr, "execve() error.\n");
            return -1;   
        }
    } else {
        if (-1 == execve(argv[1], args, envs)) {
            fprintf(stderr, "execve() error.\n"); 
            return -1;   
        }
    }

    // No need to release the memory allocated by new operator
    // We never arrived here if exec*() is successfully invoked. 
    return 0;
}
