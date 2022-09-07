/**
NOTES: 
    This is a example about "POSIX (interval) timers" usage. 
    It's extracted from linux man page.
    
Build: 
    g++  <this-file> -lrt 

Timer QuickStart: 
    The timers created by timer_create() are commonly known as "POSIX (interval) timers".  
    The POSIX timers API consists of the following interfaces:
    timer_create():     Create a timer.
    timer_settime():    Arm (start) or disarm (stop) a timer.
    timer_gettime():    Fetch the time remaining until the next expiration of a timer, along with the interval setting of the timer.
    timer_getoverrun(): Return the overrun count for the last timer expiration.
    timer_delete():     Disarm and delete a timer.
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#define CLOCKID CLOCK_REALTIME
#define SIG     SIGRTMIN 

#define errExit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

static void print_siginfo(siginfo_t* si)
{
    timer_t* tidp = (timer_t*) si->si_value.sival_ptr;
    printf("    sival_ptr = %p; ", si->si_value.sival_ptr);
    printf("    *sival_ptr = 0x%lx\n", (long) *tidp);
    int overrun = timer_getoverrun(*tidp);
    if (overrun == -1)
        errExit("timer_getoverrun");
    else
        printf("    overrun count = %d\n", overrun);
}

static void handler(int sig, siginfo_t* si, void* uc)
{
    // Note: calling printf() from a signal handler is not strictly correct, 
    //       since printf() is not async-signal-safe; see signal(7)
    printf("Caught signal %d\n", sig);
    print_siginfo(si);
    signal(sig, SIG_IGN);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <sleep-secs> <freq-nanosecs>\n", argv[0]);
        fprintf(stderr, "Example: %s 1 100\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Establish handler for timer signal
    printf("Establishing handler for signal %d\n", SIG);
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIG, &sa, NULL) == -1)
        errExit("sigaction");

    // Block timer signal temporarily
    printf("Blocking signal %d\n", SIG);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
        errExit("sigprocmask");

    // Create the timer
    timer_t timerid;
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIG;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCKID, &sev, &timerid) == -1)
        errExit("timer_create");

    printf("timer ID is 0x%lx\n", (long) timerid);

    // Start the timer
    long long freq_nanosecs = atoll(argv[2]);
    struct itimerspec its;
    its.it_value.tv_sec = freq_nanosecs / 1000000000;
    its.it_value.tv_nsec = freq_nanosecs % 1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    if (timer_settime(timerid, 0, &its, NULL) == -1)
        errExit("timer_settime");

    // Sleep for a while; meanwhile, the timer may expire multiple times
    printf("Sleeping for %d seconds\n", atoi(argv[1]));
    sleep(atoi(argv[1]));

    // Unlock the timer signal, so that timer notification can be delivered
    printf("Unblocking signal %d\n", SIG);
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
        errExit("sigprocmask");

    exit(EXIT_SUCCESS);
}