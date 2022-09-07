#include <iostream>
#include <string>

using namespace std;


template <typename K, typename V>
void dump(const K& k,  const V& v) {
    cout << k << "=" << v <<endl;
}


template <typename K, typename V, typename... Args>
void dump(const K& k, const V& v, const Args&... args) {
     cout << k << "=" << v <<endl;
     dump(args...);
}

#define F1(a)               #a, a
#define F2(a, x1)           #a, a, F1(x1)
#define F3(a, x1, x2)       #a, a, F2(x1, x2)
#define F4(a, x1, x2, x3)   #a, a, F3(x1, x2, x3)

#define PRINT(...)      \
void print() {          \
    dump(__VA_ARGS__);  \
}

#define PRINT1(...)   PRINT(F1(__VA_ARGS__))
#define PRINT2(...)   PRINT(F2(__VA_ARGS__))
#define PRINT3(...)   PRINT(F3(__VA_ARGS__))
#define PRINT4(...)   PRINT(F4(__VA_ARGS__))  


struct Foo {
    char c = '1';
    int  i = 1;
    double d = 1.0;
    string s = "1";
    PRINT4(c, i, d, s)
};

struct Bar {
    char a = '2';
    int  b = 200;
    double c = 2.05;
    PRINT3(a, b, c)
};


template <int N, typename... T>
struct param;

template <typename T0, typename... T>
struct param<0, T0, T...> {
    typedef T0 type;
};

template <int N, typename T0, typename... T>
struct param<N, T0, T...> {
    typedef typename param<N-1, T...>::type type;
};


int main() {
    Foo().print();
    Bar().print();
/*
    param<0, string, int, double>::type s = "100";
    param<1, string, int, double>::type i = 100;
    param<2, string, int, double>::type d = 100.0;

    dump("s", s, "i", i, "d", d); */

    return 0;
}
