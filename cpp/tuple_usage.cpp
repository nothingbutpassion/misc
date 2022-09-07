#include <tuple>
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
    tuple<int, char, string> tp(1, 'a', "hello");
    
    get<0>(tp) = 2;
    get<2>(tp) = "hi";
    
    cout << get<0>(tp) << endl;
    cout << get<1>(tp) << endl;
    cout << get<2>(tp) << endl;
    return 0;
}
