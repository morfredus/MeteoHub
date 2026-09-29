// Tests unitaires (hôte) de la règle du démarrage à froid : conforme à la
// dernière mesure de moins de 10 min -> gardée ; sinon -> marquée. Sans matériel.
#include <unity.h>
#include "cold_boot_rule.h"

using namespace mhcold;

void setUp() {}
void tearDown() {}

static const Reading kPrev{1000, 30.1f, 58.0f, 1013.5f};

// --- Cas réel du 29/09 14:29 : 30,24 °C / 56 % / 1013,4 après 30,1 / 58 / 1013,5
void test_native_real_case_is_kept() {
    const Reading cur{1000 + 233, 30.24f, 56.1f, 1013.38f};
    TEST_ASSERT_TRUE(conformsToPrevious(true, kPrev, cur));
}

// --- Carte chauffée : +2,5 °C -> marquée ---------------------------------------
void test_native_heated_board_is_flagged() {
    const Reading cur{1000 + 300, 32.6f, 50.0f, 1013.4f};
    TEST_ASSERT_FALSE(conformsToPrevious(true, kPrev, cur));
}

// --- Dernière mesure trop ancienne (> 10 min) : pas de quoi juger -> marquée ---
void test_native_previous_too_old_is_flagged() {
    const Reading cur{1000 + 601, 30.1f, 58.0f, 1013.5f};
    TEST_ASSERT_FALSE(conformsToPrevious(true, kPrev, cur));
    const Reading justIn{1000 + 600, 30.1f, 58.0f, 1013.5f};
    TEST_ASSERT_TRUE(conformsToPrevious(true, kPrev, justIn));
}

// --- Aucune mesure précédente (hub redémarré, historique vide) -> marquée ------
void test_native_no_previous_is_flagged() {
    TEST_ASSERT_FALSE(conformsToPrevious(false, Reading{}, Reading{2000, 20.0f, 50.0f, 1010.0f}));
}

// --- Écart d'humidité ou de pression hors bruit -> marquée ---------------------
void test_native_humidity_or_pressure_gap_is_flagged() {
    TEST_ASSERT_FALSE(conformsToPrevious(true, kPrev, Reading{1300, 30.1f, 45.0f, 1013.5f}));
    TEST_ASSERT_FALSE(conformsToPrevious(true, kPrev, Reading{1300, 30.1f, 58.0f, 1016.0f}));
}

// --- Grandeur non mesurée d'un côté (0) : non jugée ----------------------------
void test_native_absent_field_not_judged() {
    TEST_ASSERT_TRUE(conformsToPrevious(true, kPrev, Reading{1300, 30.2f, 0.0f, 0.0f}));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_real_case_is_kept);
    RUN_TEST(test_native_heated_board_is_flagged);
    RUN_TEST(test_native_previous_too_old_is_flagged);
    RUN_TEST(test_native_no_previous_is_flagged);
    RUN_TEST(test_native_humidity_or_pressure_gap_is_flagged);
    RUN_TEST(test_native_absent_field_not_judged);
    return UNITY_END();
}
