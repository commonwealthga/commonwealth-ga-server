#pragma once
// Minimal self-contained test framework. TEST(name){...} auto-registers; main.cpp
// runs the registry. CHECK*/ macros record failures without aborting the run.
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace tst {

struct TestCase { std::string name; std::function<void()> fn; };
inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
inline int& failures() { static int f = 0; return f; }
inline int& checks()   { static int c = 0; return c; }

struct Registrar {
    Registrar(const std::string& n, std::function<void()> f) { registry().push_back({n, std::move(f)}); }
};

inline void report_fail(const char* file, int line, const std::string& msg) {
    failures()++;
    std::printf("    FAIL %s:%d: %s\n", file, line, msg.c_str());
}

}  // namespace tst

#define TEST(name)                                                        \
    static void name();                                                   \
    static ::tst::Registrar reg_##name(#name, name);                      \
    static void name()

#define CHECK(cond)                                                       \
    do { ::tst::checks()++;                                               \
        if (!(cond)) ::tst::report_fail(__FILE__, __LINE__, "CHECK failed: " #cond); \
    } while (0)

#define CHECK_EQ(a, b)                                                    \
    do { ::tst::checks()++; auto _va = (a); auto _vb = (b);              \
        if (!(_va == _vb)) ::tst::report_fail(__FILE__, __LINE__,        \
            std::string("CHECK_EQ failed: " #a " == " #b)); \
    } while (0)
