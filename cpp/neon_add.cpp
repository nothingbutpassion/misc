//
// Try the following cmds to build
// $CXX neon_add.cpp -o neon_add
// $CXX -O3 neon_add.cpp -o neon_add
#include <sys/time.h>
#include <cstdio>
#include <arm_neon.h>

struct Timer {
    Timer() { 
        gettimeofday(&start, nullptr);
    }
    int duration() {
        timeval end = {0};
        gettimeofday(&end, nullptr);
        return int(end.tv_sec - start.tv_sec)*1000 + (int(end.tv_usec) - int(start.tv_usec))/1000;
    }
private:
    timeval start = {0};
};


void add(int* a, int* b, int* c, int n) {
    for (int i=0; i < n; ++i)
        c[i] = a[i] + b[i];
}

void unroll_add(int* a, int* b, int* c, int n) {
    for (int i=0; i < n; i += 4) {
        c[i]   = a[i]   + b[i];
        c[i+1] = a[i+1] + b[i+1];
        c[i+2] = a[i+2] + b[i+2];
        c[i+3] = a[i+3] + b[i+3];
    }
        
}

void neon_add(int* a, int* b, int* c, int n) {
    for (int i=0; i < n; i += 4) {
        vst1q_s32(c+i, vaddq_s32(vld1q_s32(a+i), vld1q_s32(b+i)));        
    }
}


int main(int argc, char** argv) {
    int  n = 1024*1024*16;
    int* a = new int[n];
    int* b = new int[n];
    int* c = new int[n];
    {
        Timer t;
        add(a, b, c, n);
        printf("using add: %dms\n", t.duration());
    }

    {
        Timer t;
        unroll_add(a, b, c, n);
        printf("using unroll_add: %dms\n", t.duration());
    }
    
    {
        Timer t;
        neon_add(a, b, c, n);
        printf("using neon_add: %dms\n", t.duration());
    }

    delete[] a;
    delete[] b;
    delete[] c;

    return 0;
}

