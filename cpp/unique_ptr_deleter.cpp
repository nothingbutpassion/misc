#include <stdio.h>
#include <memory>


using namespace std;

int main() {

    unique_ptr<int, void(*)(int*)> p(new int(100), [](int* p){ printf("*p=%d\n", *p); delete p; });

    return 0;
};
