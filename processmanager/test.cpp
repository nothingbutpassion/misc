#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <pthread.h>
#include "framework_log.h"
#include "framework_utils.h"
#include "process.h"
#include "process_manager.h"
#include "fd_watcher.h"
#include "signal_watcher.h"

using namespace std;

const char* TAG = "main";

void* test_func(void*) {

    LOGD(TAG, "test thread is entered");
    
    string file = "/bin/sh";
    vector<string> args = { "sh", "-c", "echo hello stdout; sync; sleep 4; echo hello stderr 1>&2"};
    pid_t pid = ProcessManager::instance().create(file, args, "out.txt", "err.txt");

    while (true) {
        char buf[256];
        ssize_t len = ProcessManager::instance().recvData(pid, buf, sizeof(buf), 1000);
        if (len == -1) {
            LOGD(TAG, "test thread recvData error.");
            break;
        } else if (len > 0) { 
            LOGD(TAG, "test thread read %d bytes from stdout", len);
            len = write(STDERR_FILENO, buf, len);
            LOGD(TAG, "test thread write %d bytes to stderr", len);
        }
    }

    string file2 = "/bin/sh";
    vector<string> args2 = { "sh", "print_stdin.sh" };
    pid_t pid2 = ProcessManager::instance().create(file2, args2, "out2.txt", "err2.txt");

    while (true) {
        // first time talk with child
        string cmd = "hello, child\nhi, child\nTEST TEST TEST";
        int len = ProcessManager::instance().sendData(pid2, cmd.data(), cmd.size());
        if (len == -1) {
            LOGD(TAG, "test thread sendData error.");
            break;
        } 


        // second time talk with child
        cmd = "\nhello, xxx child\nhi, xxxx child\nstop\n";
        len = ProcessManager::instance().sendData(pid2, cmd.data(), cmd.size());
        if (len == -1) {
            LOGD(TAG, "test thread sendData error.");
            break;
        } 

        while (true) {
            char buf[256];
            int len = ProcessManager::instance().recvData(pid2, buf, sizeof(buf));
            if (len == -1) {
                LOGD(TAG, "test thread recvData error.");
                break;
            } else if (len > 0) { 
                LOGD(TAG, "test thread read %d bytes from stdout", len);
                len = write(STDERR_FILENO, buf, len);
                LOGD(TAG, "test thread write %d bytes to stderr", len);
            }
        }
        break;

    }

    LOGD(TAG, "test thread is exited");
    return 0;

    
}


int main() {
    LOGD(TAG, "main thead is entered");   
    ProcessManager::instance().start();

    pthread_t tid;
    pthread_create(&tid, NULL, test_func, NULL);
    pthread_join(tid, NULL);
    
    ProcessManager::instance().stop();
    LOGD(TAG, "main thead is exited");
    return 0;
}




