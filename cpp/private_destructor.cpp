#include <stdio.h>


struct Foo {
    
    static Foo& instance() {
        static Foo foo;
        return foo;    
    }

private:
    Foo() { printf("Foo() called\n"); }
    ~Foo() { printf("~Foo() called\n"); }
};


int main() {
    Foo::instance();
    return 0;
}
