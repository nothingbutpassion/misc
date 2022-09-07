#include <stdio.h>


struct Color {
     bool red: 1;
     bool green: 1;
     bool blue: 1;
     bool : 0;
     bool y: 1;
     bool u: 1;
     bool v: 1;
};


int main() {

    printf("sizeof(bool)=%ld\n", sizeof(bool));
    printf("sizeof(Color)=%ld\n", sizeof(Color));
    return 0;
};
