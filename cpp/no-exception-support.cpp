/**
 * Try the follwing 2 ways to build/run this app:
 * 1) g++ -fno-exceptions <this-cpp-file>
 * 2) g++ -fno-exceptions <this-cpp-file>
 */

#include <stdio.h>
#include <vector>

int main(int argc, char *argv[])
{
    // NOTES: 
    // STL code doesn't produce compile error if "-fno-exceptions" passed to g++
    // Exception handling in STL are removed; throws are replaced with abort() calls.
    std::vector<int> v(10000000000000000);
    printf("v.size = %lld\n", v.size());

    // NOTES:
    // std::new doesn't produce compile error if "-fno-exceptions" passed to g++
    ::operator new (10000000000000000);

    //
    // NOTES: 
    // User code that use try/catch/throw will produce compile error if "-fno-exceptions" passed to g++
    // Uncomment the following code to have a try.
    // 
    // try {
    //     ::operator new (10000000000000000);
    // }
    // catch (...) {
    //     printf("catch new exception: %s, %d, %s\n", __FILE__, __LINE__, __func__);
    // }

    return 0;
}