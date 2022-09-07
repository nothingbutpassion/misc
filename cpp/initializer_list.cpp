#include <iostream>
#include <initializer_list>

using namespace std;

struct myclass {
    myclass(int n, int v) : x(0), y(0), z(0) {
        cout << "myclass(int, int) : ";
        for(int i=0; i < n; i++) {
            cout << v << " ";
        }
        cout << endl;
    }

    myclass(initializer_list<int> l) : x(1), y(1), z(1) {
        cout << "myclass(initializer_list<int>) : ";
        for(auto i=l.begin(); i != l.end(); ++i) {
            cout << *i << " ";
        }
        cout << endl;
    }

    void print() { 
        cout << "x = " << x << ", y = " << y << ", z = " << z << endl;
    }

    int x;
    int y;
    int z;
};

struct mystruct {
    void print() { 
        cout << "x = " << x << ", y = " << y << ", z = " << z << endl;
    }

    int x;
    int y;
    int z;
};


struct normalclass {
    normalclass(int n, int v) : x(0), y(0), z(0) {
        cout << "myclass(int, int) : ";
        for(int i=0; i < n; i++) {
            cout << v << " ";
        }
        cout << endl;
    }

    void print() { 
        cout << "x = " << x << ", y = " << y << ", z = " << z << endl;
    }

    int x;
    int y;
    int z;
};

//
// Note:
// 1) T t{...} and T t = {...} has the same effect
// 2) T t{...} and T t(...) has the same effect if there's no initializer_list constructor
// 3) T t = {...} has the old struct initialization effect if there's no constructor
// 
int main(int argc, char** argv) 
{
    myclass a{10, 20, 30};          // calls initializer_list constructor
    a.print();                      // x = 1, y = 1, z = 1
    myclass b = {10, 20, 30};       // same as above. because T t{...} is equal to T t = {...} in c++11.
    b.print();                      // x = 1, y = 1, z = 1
    myclass c(10, 20);              // calls first constructor
    c.print();                      // x = 0, y = 0, z = 0

    mystruct a2{10, 20, 30};        // old struct initialization style
    a2.print();                     // x = 10, y = 20, z = 30
    mystruct b2 = {10};             // old struct initialization style
    b2.print();                     // x = 10, y = 0, z = 0

    normalclass a3{10, 20};         // call myclass(int, int)
    a3.print();                     // x = 0, y = 0, z = 0
    normalclass b3 = {10, 20};      // call myclass(int, int)
    b3.print();                     // x = 0, y = 0, z = 0
    normalclass c3(10, 20);         // call myclass(int, int)
    c3.print();                     // x = 0, y = 0, z = 0
    return 0;
}


