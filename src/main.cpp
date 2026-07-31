#include "core/Application.hpp"
#include <cstdio>

int main(int /*argc*/, char* /*argv*/[]) {
    try {
        Application app("MonoPoly IDE  v0.1", 1600, 900);
        app.Run();
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "[FATAL] %s\n", ex.what());
        return 1;
    }
    return 0;
}
