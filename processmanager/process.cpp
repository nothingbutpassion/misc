#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "process.h"
#include "framework_log.h"
#include "framework_utils.h"

using namespace std;

// tag for this file
#define TAG "Process"


bool Process::start(const std::string& file, const std::vector<std::string>& args, 
           const std::string& stdinFile, const std::string& stdoutFile, const std::string& stderrFile) {
    enum { RD, WR };
    int pexec[2] = {-1, -1};
    if (pipe(pexec) == -1) {
        LOGE(TAG, "create pipe failed: %s", strerror(errno));
        return false;
    }
    if (fcntl(pexec[RD], F_SETFD, FD_CLOEXEC) == -1 
        || fcntl(pexec[WR], F_SETFD, FD_CLOEXEC) == -1) {
        LOGE(TAG, "fcntl pipe failed: %s", strerror(errno));
        closeFDs({pexec[RD], pexec[WR]});
        return false;   
    }

   
    pid_t pid = fork(stdinFile, stdoutFile, stderrFile);
    if (pid == -1) {
        closeFDs({pexec[RD], pexec[WR]});
        return false;
    }

    if (pid == 0) {
        // close all fds except stdin, stdout, stderr, and pexec[WR]
        closeAllFDs({STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO, pexec[WR]});

        
        // This is in child process
        char** argv = new char*[args.size()+1];
        for (size_t i = 0; i < args.size(); ++i) {
            const string& src = args[i];            
            char*& dest = argv[i];
            dest = new char[src.size()+1];
            dest[src.copy(dest, src.size())] = '\0';
        }
        argv[args.size()] = NULL;
        execvp(file.c_str(), argv);
        
        // execvp() failed.
        //LOGE(TAG, "execvp failed: %s", strerror(errno));
        int error = errno;
        while (write(pexec[WR], &error, sizeof(error)) == -1 
            && errno == EINTR){ 
        }
        close(pexec[WR]);
        exit(error); 
    }

    // This is in parent process
    close(pexec[WR]);
    int error;
    int ret = read(pexec[RD], &error, sizeof(error));
    if (ret > 0 || ret == -1) {
        closeFDs({infd, outfd, errfd, pexec[RD]});
        return false;
    }

    // succeed
    close(pexec[RD]);
    this->file = file;
    this->args = args;
    return true;
}


pid_t Process::fork(const string& stdinFile, const string& stdoutFile, const string& stderrFile) {

    int inFD = -1;
    int outFD = -1;
    int errFD = -1;
    int pin[2] = {-1, -1};
    int pout[2] = {-1, -1};
    int perr[2] = {-1, -1};

    enum { RD, WR };

    // stdin
    if (!stdinFile.empty()) {
        inFD = open(stdinFile.c_str(), O_RDWR|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
        if (inFD == -1) {
            LOGE(TAG, "open %s failed: %s", stdinFile.c_str(), strerror(errno));
            return -1;
        }
    } else {
        if (pipe(pin) == -1) {
            LOGE(TAG, "create pipe failed: %s", strerror(errno));
            return -1;
        }
    }

    // stdout
    if (!stdoutFile.empty()) {
        outFD = open(stdoutFile.c_str(), O_RDWR|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
        if (outFD == -1) {
            LOGE(TAG, "open %s failed: %s", stdoutFile.c_str(), strerror(errno));
            closeFDs({inFD, outFD, errFD, pin[RD], pin[WR], pout[RD], pout[WR], perr[RD], perr[WR]});
            return -1;
        }
    } else {
        if (pipe(pout) == -1) {
            LOGE(TAG, "create pipe failed: %s", strerror(errno));
            closeFDs({inFD, outFD, errFD, pin[RD], pin[WR], pout[RD], pout[WR], perr[RD], perr[WR]});
            return -1;
        } 
    }

    // stderr
    if (!stderrFile.empty()) {
        errFD = open(stderrFile.c_str(), O_RDWR|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
        if (errFD == -1) {
            LOGE(TAG, "open %s failed: %s", stderrFile.c_str(), strerror(errno));
            return -1;
        }
    } else {
        if (pipe(perr) == -1) {
            LOGE(TAG, "create pipe failed: %s", strerror(errno));
            closeFDs({inFD, outFD, errFD, pin[RD], pin[WR], pout[RD], pout[WR], perr[RD], perr[WR]});
            return false;
        } 
    }

    // fork a child process
    pid_t pid = ::fork();
    if (pid == -1) {
        LOGE(TAG, "fork failed: %s", strerror(errno));
        closeFDs({inFD, outFD, errFD, pin[RD], pin[WR], pout[RD], pout[WR], perr[RD], perr[WR]});
        return false;
    }

    if (pid == 0) {
        // This is in child process
        if (inFD != -1) {
            dup2(inFD, STDIN_FILENO);
            close(inFD);
        } else {
            close(pin[WR]);
            dup2(pin[RD], STDIN_FILENO);
            close(pin[RD]);
        }
        if (outFD != -1) {
            dup2(outFD, STDOUT_FILENO);
            close(outFD);
        } else {
            close(pout[RD]);
            dup2(pout[WR], STDOUT_FILENO);
            close(pout[WR]);
        }

        if (errFD != -1) {
            dup2(errFD, STDERR_FILENO);
            close(errFD);
        } else {
            close(perr[RD]);
            dup2(perr[WR], STDERR_FILENO);
            close(perr[WR]); 
        }

        return pid;
    }

    
    // This is in parent process
    if (inFD != -1) {
        infd = open(stdinFile.c_str(), O_WRONLY);
        if (infd == -1) {
            LOGE(TAG, "open %s failed: %s", stdinFile.c_str(), strerror(errno));
            infd = inFD;
        } else {
            close(inFD);
        }
    } else {
        close(pin[RD]);
        infd = pin[WR];
    }
    
    if (outFD != -1) {
        outfd = open(stdoutFile.c_str(), O_RDONLY);
        if (outfd == -1) {
            LOGE(TAG, "open %s failed: %s", stdoutFile.c_str(), strerror(errno));
            outfd = outFD;
        } else {
            close(outFD);
        }
    } else {
        close(pout[WR]);
        outfd = pout[RD];        
    }
    
    if (errFD != -1) {
        errfd = open(stderrFile.c_str(), O_RDONLY);
        if (errfd == -1) {
            LOGE(TAG, "open %s failed: %s", stdoutFile.c_str(), strerror(errno));
            errfd = errFD;
        } else {
            close(errFD);
        }
    } else {
        close(perr[WR]);
        errfd = perr[RD];
    }
    
    this->pid = pid;
    LOGD(TAG, "child process: pid=%d, pin=[%d,%d] pout=[%d,%d] perr=[%d,%d], inFD=%d outFD=%d errFD=%d, "
        "infd=%d outfd=%d errfd=%d, inFile=%s outFile=%s errFile=%s",
        pid, pin[RD], pin[WR], pout[RD], pout[WR], perr[RD], perr[WR], inFD, outFD, errFD, 
        infd, outfd, errfd, stdinFile.c_str(), stdoutFile.c_str(), stderrFile.c_str());
    return pid;
}


void Process::closeFDs(const vector<int>& fds) {
    for (int fd : fds) {
        if (fd != -1) {
            close(fd);
            //LOGD(TAG, "closeFDs(): close fd %d", fd);
        }
    }
}


bool Process::closeAllFDs(const set<int>& excludeFDs) {
    vector<string> fdNames = dirEnties("/proc/self/fd");
    if (!fdNames.empty()) {
        for (const string& fdName: fdNames) {
            int fd = stoi(fdName);
            //LOGD(TAG, "closeAllFDs(): find fd %d", fd);
            if (excludeFDs.find(fd) == excludeFDs.end()) {
                close(fd);
                //LOGD(TAG, "closeAllFDs(): close fd %d", fd);
            }
        }
        return true;
    }
    return false;
}

void Process::dump() {
    string s = "";
    for (auto it = args.begin(); it != args.end(); ++it) {
        s += *it; 
        s += " ";
    }   
    LOGD(TAG, "child process: pid=%d, infd=%d, outfd=%d, errfd=%d, file=%s, args=%s", 
        pid, infd, outfd, errfd, file.c_str(), s.c_str());
}







