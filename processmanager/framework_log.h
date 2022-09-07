#pragma once

#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>


#ifndef LOG_LEVEL       
#define LOG_LEVEL       1
#endif

#define LOG_DEBUG       1
#define LOG_INFO        2
#define LOG_WARN        3
#define LOG_ERROR       4

#if LOG_ERROR >= LOG_LEVEL
#define LOGE(tag, ...)   printLog("ERROR", tag, __VA_ARGS__)                  
#else
#define LOGE(...)
#endif

#if LOG_WARN >= LOG_LEVEL
#define LOGW(tag, ...)   printLog("WARNING", tag, __VA_ARGS__)                  
#else
#define LOGW(...)
#endif 

#if LOG_INFO >= LOG_LEVEL
#define LOGI(tag, ...)   printLog("INFO", tag, __VA_ARGS__)                  
#else
#define LOGI(...)
#endif 

#if LOG_DEBUG >= LOG_LEVEL
#define LOGD(tag, ...)   printLog("DEBUG", tag,  __VA_ARGS__)                  
#else
#define LOGD(...)
#endif


#define LOG_BUF_SIZE    1024


inline void printLog(const char* priority, const char* tag, const char *fmt, ...) {
    // format log str
    va_list ap;
    char logstr[LOG_BUF_SIZE];
    va_start(ap, fmt);
    vsnprintf(logstr, sizeof(logstr), fmt, ap);
    va_end(ap);

    // format time str
    char timestr[32];
    timeval tv;
    gettimeofday(&tv, NULL);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
   
    // write format str to stderr
    fprintf(stderr, "%s.%03d %d %d %s %s %s\n", timestr, int(tv.tv_usec/1000), getpid(), int(syscall(SYS_gettid)), priority, tag, logstr);
}


