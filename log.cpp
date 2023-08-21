#include <iostream>

#include "log.h"

namespace fxdsp {

void print_floats(const std::vector<float>& values) {
    std::cout << "[";
    for (auto i = 0; i < values.size(); i++) {
        std::cout << values[i];
        if (i != values.size() - 1) {
            std::cout << ',';
        }
    }
    std::cout << "]\n";
}

}
