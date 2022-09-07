#include <stdio.h>
#include <sys/time.h>       // for gettimeofday()
#include <time.h>           // for time()/ctime()/gmtime()/localtime()/mktime()/


int main(int argc, char** argv) {

    // getdayoftime(): get seconds and microseconds since the Epoc
    // struct timeval {
    //          time_t      tv_sec;     /* seconds */
    //          suseconds_t tv_usec;    /* microseconds */
    // }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("gettimeofday(): tv_sec=%llu, tv_usec=%llu\n", tv.tv_sec, tv.tv_usec);
    

    // time() : get seconds since Epoc
    time_t t = time(NULL);
    printf("time(): %llu\n", t);

    // ctime() : convert time_t to c-string
    char* ct = ctime(&t);
    printf("ctime(): %s", ct);


    // gmtime()/localtime(): convert time_t to tm
    // struct tm {
    //           int tm_sec;         /* seconds */
    //           int tm_min;         /* minutes */
    //           int tm_hour;        /* hours */
    //           int tm_mday;        /* day of the month */
    //           int tm_mon;         /* month */
    //           int tm_year;        /* year */
    //           int tm_wday;        /* day of the week */
    //           int tm_yday;        /* day in the year */
    //           int tm_isdst;       /* daylight saving time */
    //  };
    struct tm* gtm = gmtime(&t);
    struct tm* ltm = localtime(&t);

    // asctime(): covert tm to c-strng
    char* cg = asctime(gtm);
    char* cl = asctime(ltm);
    printf("asctime(): %s", cg);
    printf("asctime(): %s", cl);

    // mktime(): convert tm to time_t
    time_t t1 = mktime(gtm);
    time_t t2 = mktime(ltm);
    printf("mktime(): %llu\n", t1);
    printf("mktime(): %llu\n", t2);

    // strftime(): covert tm to <user string>
    char ut[256];
    strftime(ut, sizeof(ut), "%Y-%m-%dT%H:%M:%Sz", gtm);
    printf("strftime(): %s\n", ut);
    strftime(ut, sizeof(ut), "%Y-%m-%dT%H:%M:%Sz", ltm);
    printf("strftime(): %s\n", ut);

    // strptime(): convert <user string> to tm
    struct tm utm;
    strptime(ut, "%Y-%m-%dT%H:%M:%Sz", &utm);
    printf("strptime(): %llu\n", mktime(&utm));

    return 0;
}
