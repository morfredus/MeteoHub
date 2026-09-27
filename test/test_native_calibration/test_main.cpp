// Tests unitaires (hôte) de la calibration du capteur intérieur : décalages
// indépendants, bornes, humidité maintenue dans [0, 100]. Sans matériel.
#include <unity.h>
#include "sensor_calibration.h"

using namespace mhcal;

void setUp() {}
void tearDown() {}

// --- Par défaut, aucune correction ------------------------------------------
void test_native_default_is_identity() {
    const Offsets o;
    TEST_ASSERT_EQUAL_FLOAT(26.8f, correctTemperature(26.8f, o));
    TEST_ASSERT_EQUAL_FLOAT(67.0f, correctHumidity(67.0f, o));
}

// --- Un décalage de température ne touche pas l'humidité ---------------------
void test_native_temperature_only() {
    Offsets o;
    o.temperature = -2.8f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 24.0f, correctTemperature(26.8f, o));
    // L'humidité n'est PAS recalculée à partir de la température corrigée.
    TEST_ASSERT_EQUAL_FLOAT(60.0f, correctHumidity(60.0f, o));
}

// --- Cas de terrain : hub 26,8 °C / 60 %, thermostat 24,0 °C / 67 % ----------
void test_native_field_case_both_offsets() {
    Offsets o;
    o.temperature = -2.8f;
    o.humidity = 7.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 24.0f, correctTemperature(26.8f, o));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 67.0f, correctHumidity(60.0f, o));
}

// --- L'humidité corrigée reste une humidité relative plausible --------------
void test_native_humidity_clamped() {
    Offsets o;
    o.humidity = 10.0f;
    TEST_ASSERT_EQUAL_FLOAT(100.0f, correctHumidity(95.0f, o));
    o.humidity = -10.0f;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, correctHumidity(4.0f, o));
}

// --- Une saisie aberrante est bornée, une valeur non numérique annulée ------
void test_native_sanitize_bounds() {
    Offsets o;
    o.temperature = -25.0f;
    o.humidity = 40.0f;
    const Offsets s = sanitize(o);
    TEST_ASSERT_EQUAL_FLOAT(-kTempOffsetMax, s.temperature);
    TEST_ASSERT_EQUAL_FLOAT(kHumOffsetMax, s.humidity);

    Offsets n;
    n.temperature = 0.0f / 0.0f;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, sanitize(n).temperature);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_default_is_identity);
    RUN_TEST(test_native_temperature_only);
    RUN_TEST(test_native_field_case_both_offsets);
    RUN_TEST(test_native_humidity_clamped);
    RUN_TEST(test_native_sanitize_bounds);
    return UNITY_END();
}
