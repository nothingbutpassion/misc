#include <signal.h>
#include <memory>
#include <iostream>

int main() {
    std::shared_ptr<void(int)> handler(
        signal(SIGFPE, [](int signum) {throw std::logic_error("Custom FPE\n"); }),
        [](__sighandler_t f) { signal(SIGFPE, f); });

    int i = 0;

    std::cin >> i;  // what if someone enters zero?

    try {
        i = 5/i;
    }
    catch (std::logic_error e) {
        std::cerr << e.what();
    }
    std::cout << "normal exit." << std::endl;
    return 0;
}
