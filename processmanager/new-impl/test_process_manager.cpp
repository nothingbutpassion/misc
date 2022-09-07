#include <vector>
#include <string>

#include "utils.h"
#include "log.h"
#include "process.h"

#define TAG "main"

using namespace std;

int main() {
    
    string file = "/bin/sh";
    vector<string> args = {"sh", "-c", "sleep 1; read x; echo $x; echo hello err 1>&2; sleep 3"};

    Process p1;
    p1.create(file, args, "in.txt", "out.txt", "err.txt");
    p1.dump();

    vector<string> a2 = {"sh", "-c", "echo hello out > in.txt"};
    Process p2;
    p2.create(file, a2, "", "", "");
    p2.dump();

    
    LOGD("main", "p1.isAlive(): %d", p1.isAlive() ? 1 : 0);
    LOGD("main", "p2.isAlive(): %d", p2.isAlive() ? 1 : 0);
    sleep(3);
    LOGD("main", "p1.isAlive(): %d", p1.isAlive() ? 1 : 0);
    LOGD("main", "p2.isAlive(): %d", p2.isAlive() ? 1 : 0);
    p1.kill();
    sleep(1);
    LOGD("main", "p1.isAlive(): %d", p1.isAlive() ? 1 : 0);
    LOGD("main", "p2.isAlive(): %d", p2.isAlive() ? 1 : 0);
    
    return 0;
}
