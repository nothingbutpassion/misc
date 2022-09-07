#include <stdio.h>


int my_add(int a, int b) {
    int i=0;
    for(; i < b; i++) {
        a++;
    }
    return a;
}

int my_mul(int a, int b) {
    int s = 0;
    int i = 0;
    for(; i < a; i++) {
	s = my_add(s, b);
    }
    return s;
}


int main(int argc, char** argv) {

    int a = 10000;
    int b = 10000;
    printf("my_mul(%d, %d)=%d\n", a, b, my_mul(a, b));

    return 0;
}
