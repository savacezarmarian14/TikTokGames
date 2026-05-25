#include "utils/IdGenerator.hpp"

namespace tw {

int IdGenerator::current_ = 0;

int IdGenerator::next() {
    return current_++;
}

void IdGenerator::reset() {
    current_ = 0;
}

} // namespace tw
