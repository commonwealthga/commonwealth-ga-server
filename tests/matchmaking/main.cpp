#include "check.hpp"

int main() {
    int run = 0;
    for (auto& t : tst::registry()) {
        const int before = tst::failures();
        t.fn();
        run++;
        if (tst::failures() == before)
            std::printf("[ OK ] %s\n", t.name.c_str());
        else
            std::printf("[FAIL] %s\n", t.name.c_str());
    }
    std::printf("\n%d test(s), %d check(s), %d failure(s)\n",
                run, tst::checks(), tst::failures());
    return tst::failures() ? 1 : 0;
}
