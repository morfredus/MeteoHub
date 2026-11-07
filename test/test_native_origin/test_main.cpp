// Garde anti-CSRF des ecritures (include/origin_guard.h).
#include <unity.h>
#include "origin_guard.h"

using namespace mhorigin;

void setUp() {}
void tearDown() {}

void test_native_no_origin_passes() {
    TEST_ASSERT_TRUE(writeAllowed("", "meteohub.local"));        // curl, scripts, services du parc
}

void test_native_same_origin_passes() {
    TEST_ASSERT_TRUE(writeAllowed("http://meteohub.local", "meteohub.local"));
    TEST_ASSERT_TRUE(writeAllowed("http://meteohub.local:80", "meteohub.local"));
    TEST_ASSERT_TRUE(writeAllowed("http://192.168.1.50", "192.168.1.50:80"));
    TEST_ASSERT_TRUE(writeAllowed("http://MeteoHub.local", "meteohub.local"));
}

void test_native_cross_origin_refused() {
    TEST_ASSERT_FALSE(writeAllowed("http://evil.example", "meteohub.local"));
    TEST_ASSERT_FALSE(writeAllowed("http://meteohub.local:8080", "meteohub.local"));
    TEST_ASSERT_FALSE(writeAllowed("https://meteohub.local", "meteohub.local")); // port 443 != 80
    TEST_ASSERT_FALSE(writeAllowed("null", "meteohub.local"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_no_origin_passes);
    RUN_TEST(test_native_same_origin_passes);
    RUN_TEST(test_native_cross_origin_refused);
    return UNITY_END();
}
