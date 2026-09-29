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

// --- Pression : décalage additif, jamais appliqué à une pression absente -----
void test_native_pressure_offset() {
    Offsets o;
    o.pressure = 1.5f;
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1004.5f, correctPressure(1003.0f, o));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, correctPressure(0.0f, o));   // BMP280 muet : reste 0
    // La pression ne touche ni la température ni l'humidité.
    TEST_ASSERT_EQUAL_FLOAT(20.0f, correctTemperature(20.0f, o));
    TEST_ASSERT_EQUAL_FLOAT(50.0f, correctHumidity(50.0f, o));
}

// --- Bornes pression et altitude ---------------------------------------------
void test_native_pressure_altitude_bounds() {
    Offsets o;
    o.pressure = 30.0f;
    o.altitude = 9000.0f;
    Offsets s = sanitize(o);
    TEST_ASSERT_EQUAL_FLOAT(kPresOffsetMax, s.pressure);
    TEST_ASSERT_EQUAL_FLOAT(kAltitudeMax, s.altitude);
    o.altitude = -2000.0f;
    TEST_ASSERT_EQUAL_FLOAT(kAltitudeMin, sanitize(o).altitude);
    o.altitude = 245.0f;   // valeur réaliste : conservée telle quelle
    TEST_ASSERT_EQUAL_FLOAT(245.0f, sanitize(o).altitude);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_default_is_identity);
    RUN_TEST(test_native_temperature_only);
    RUN_TEST(test_native_field_case_both_offsets);
    RUN_TEST(test_native_humidity_clamped);
    RUN_TEST(test_native_sanitize_bounds);
    RUN_TEST(test_native_pressure_offset);
    RUN_TEST(test_native_pressure_altitude_bounds);
    return UNITY_END();
}
