#include <stdio.h>
#include <utility>

//#define FOO_UNIVERSE_REF

struct Foo {
    Foo() : data(0) { 
        printf("Foo()\n");   
    }
    
    Foo(const Foo& foo) : data(foo.data) {
        printf("Foo(const Foo&), foo adrress: %p\n", &foo);  
    }

#ifdef FOO_UNIVERSE_REF
    Foo(Foo&& foo) : data(foo.data) {
        printf("Foo(Foo&&), foo adrress: %p\n", &foo);  
    } 
#endif

    int data;
};


Foo fun() {
    Foo foo;
    printf("foo address in fun: %p\n", &foo);
    return foo;
}

int main(int argc, char** argv) {
    Foo foo = fun();
    printf("foo address in main: %p\n", &foo);

    Foo foo2 = std::move(fun());
    printf("foo2 address in main: %p\n", &foo2);

    Foo foo3 = std::forward<Foo>(fun());
    printf("foo3 address in main: %p\n", &foo3);
    return 0;
}
