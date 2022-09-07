#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "vpu_utils.h"


#define LOG_BUF_SIZE    1024
#define ERR_BUF_SIZE    1024

void vpu_print_log(const char* priority, const char* tag, const char *fmt, ...) {
    // Format log str
    va_list ap;
    char logstr[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(logstr, sizeof(logstr), fmt, ap);
    va_end(ap);

    // Format time str
    char timestr[32];
    timeval tv;
    gettimeofday(&tv, NULL);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
   
    // Write format str to stderr
    fprintf(stderr, "%s.%03d %d %d %s %s %s\n", timestr, int(tv.tv_usec/1000), getpid(), int(syscall(SYS_gettid)), priority, tag, logstr);
}


static char vpu_error_buf[ERR_BUF_SIZE];

void vpu_set_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(vpu_error_buf, sizeof(vpu_error_buf), fmt, ap);
    va_end(ap);   
}

const char* vpu_get_error() {
    return vpu_error_buf;
}




