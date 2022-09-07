#include <iostream>     // std::cout
#include <functional>   // std::function, std::plus, std::minus, std::multiplies



int divid(int a, int b) {
    return a/b;
}

struct Mode {
    int operator () (int a, int b) {
        return a%b;
    }
};

int main () {
  // an array of functions:
  std::function<int(int,int)> fn[] = {
    std::plus<int>(),
    std::minus<int>(),
    std::multiplies<int>(),
    divid,
    Mode()
  };

  for (auto& x: fn) std::cout << x(15,5) << '\n';

  return 0;
}
