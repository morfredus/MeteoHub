// Plages de plausibilite des mesures archivees (include/measure_range.h).
#include <unity.h>
#include "measure_range.h"

using namespace mhrange;

void setUp() {}
void tearDown() {}

void test_native_unmeasured_is_not_stored() {
    // Sonde muette : valeur 0.0 sans drapeau -> NaN, jamais 0.
    TEST_ASSERT_TRUE(std::isnan(humidityToStore(false, 0.0f)));
    TEST_ASSERT_TRUE(std::isnan(pressureToStore(false, 0.0f)));
    // Drapeau present mais valeur absurde -> NaN aussi.
    TEST_ASSERT_TRUE(std::isnan(pressureToStore(true, 0.0f)));
    TEST_ASSERT_TRUE(std::isnan(humidityToStore(true, 0.0f)));
    TEST_ASSERT_TRUE(std::isnan(pressureToStore(true, NAN)));
}

void test_native_real_values_kept() {
    TEST_ASSERT_EQUAL_FLOAT(1013.2f, pressureToStore(true, 1013.2f));
    TEST_ASSERT_EQUAL_FLOAT(55.0f, humidityToStore(true, 55.0f));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, humidityToStore(true, 100.0f));
}

void test_native_legacy_zero_ignored_when_read() {
    // Un 0.0 deja ecrit dans un .bin reste lisible mais n'est pas une mesure.
    TEST_ASSERT_FALSE(storedPressureOk(0.0f));
    TEST_ASSERT_FALSE(storedHumidityOk(0.0f));
    TEST_ASSERT_FALSE(storedPressureOk(NAN));
    TEST_ASSERT_TRUE(storedPressureOk(990.0f));
    TEST_ASSERT_TRUE(storedHumidityOk(42.0f));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_unmeasured_is_not_stored);
    RUN_TEST(test_native_real_values_kept);
    RUN_TEST(test_native_legacy_zero_ignored_when_read);
    return UNITY_END();
}
