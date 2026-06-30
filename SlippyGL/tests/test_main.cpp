#include "check.hpp"
#include <cstdio>

void test_tilemath();
void test_tilekey();
void test_tilegrid();
void test_camera();

int main()
{
    std::printf("Running SlippyGL unit tests\n");
    std::printf("===========================\n");
    test_tilemath();
    test_tilekey();
    test_tilegrid();
    test_camera();
    std::printf("---------------------------\n");
    std::printf("%d checks, %d failures\n", slippytest::g_checks, slippytest::g_fails);
    std::printf("RESULT: %s\n", slippytest::g_fails == 0 ? "PASS" : "FAIL");
    return slippytest::g_fails == 0 ? 0 : 1;
}
