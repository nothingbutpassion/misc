#include <stdio.h>

#define SIZE 0x1000000

struct Foo {

   static Foo* instance() {
      static Foo foo;
      return &foo;
   }

   Foo() {
      printf("Foo: data[0]=%d\n", data[0]);
   }
   ~Foo() {
      printf("~Foo: data[%d]=%d\n", SIZE-1, data[SIZE-1]);
   }

   // Use std=c++11 option to avoid compile error
   int data[SIZE] = {1};
};

int main()
{
   printf("before Foo::instance()\n");
   Foo::instance();
   printf("after Foo::instance()\n");

   // Uncomment the following would lead stack overflow or segmentation fault (core dumped). 
   //Foo foo;
   return 0;
}
