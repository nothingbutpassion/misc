#include <stdio.h>


class B;

class A {
public:
    void f(B& b);
};

class B {
public:
    B(): x(100) {}
    friend void A::f(B& b);
private:
    int x;
};

void A::f(B& b) {
    printf("b.x=%d\n", b.x);
}


int main() {
    A a;
    B b;
    a.f(b);
    return 0;
}
