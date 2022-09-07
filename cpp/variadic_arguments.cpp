#include <cstdarg>
#include <cstdio>


void print_ints(int first, ...) {
    va_list ap, vl;
    va_start(ap, first);    // NOTE: first is not contained in ap
    va_copy(vl, ap);        // vl is copyed from ap
    int arg_count = 0;
    while (true) {
        int val = va_arg(ap, int);       
        if (val == 0) {     // 0 is used as the ending flags 
            break;
        } else {
            arg_count++;
            printf("[%d]\n", val);        
        }       
    }
    va_end(ap);

    for(int i=0; i < arg_count; i++) {
        int val = va_arg(vl, int);
        printf("{%d}\n", val);
    }
    va_end(vl);
}


void invoke_printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);   // printf will lead incorrect outputs
    va_end(ap);
}



int main(int argc, char** argv) {
    // NOTE: zero buf will be located .bss section of the exectuable binary       
    static char buf1[1024*1024]={0};

    // NOTE: non-zero buf will be located at .data section, this will increase the build time and final binary size
    static char buf2[1024*1024]={1};
    
    print_ints(1, 20, 300, 0);
    invoke_printf("[%d][%d]\n", 400, 500);
    printf("[%d][%d]\n", 400, 500);
    return 0;
}

