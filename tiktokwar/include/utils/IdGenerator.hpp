#pragma once

namespace tw {

class IdGenerator {
public:
    static int next();
    static void reset();

private:
    static int current_;
};

} // namespace tw
