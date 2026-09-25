// Tests unitaires (hôte) de la décision d'alerte batterie de la sonde :
// paliers en tension, anti-rebond, hystérésis, reprise après redémarrage.
// Sans matériel ni réseau.
#include <unity.h>
#include "battery_alert_logic.h"

using namespace mhbat;

static const Thresholds kT = {3.40f, 3.20f, 3.80f, 3};

static void feed(BatteryAlertLogic& a, float v, int n) {
    for (int i = 0; i < n; i++) a.onReading(v);
}

void setUp() {}
void tearDown() {}

// --- Accu en forme : aucune alerte ------------------------------------------
void test_native_healthy_no_alert() {
    BatteryAlertLogic a(kT);
    feed(a, 3.95f, 10);
    TEST_ASSERT_EQUAL_UINT8(LEVEL_OK, a.observed());
    TEST_ASSERT_FALSE(a.hasPending());
}

// --- Une lecture basse isolée (appel de courant) ne déclenche rien ----------
void test_native_single_dip_ignored() {
    BatteryAlertLogic a(kT);
    feed(a, 3.70f, 3);
    a.onReading(3.10f);
    a.onReading(3.70f);
    a.onReading(3.30f);
    a.onReading(3.30f);
    a.onReading(3.70f);
    TEST_ASSERT_EQUAL_UINT8(LEVEL_OK, a.observed());
    TEST_ASSERT_FALSE(a.hasPending());
}

// --- Trois lectures sous 3,40 V : palier LOW, notifié une seule fois --------
void test_native_low_after_confirm() {
    BatteryAlertLogic a(kT);
    feed(a, 3.38f, 2);
    TEST_ASSERT_FALSE(a.hasPending());
    a.onReading(3.37f);
    TEST_ASSERT_EQUAL_UINT8(LEVEL_LOW, a.observed());
    TEST_ASSERT_TRUE(a.hasPending());
    a.markNotified();
    feed(a, 3.36f, 20);  // la tension reste basse : pas de nouvelle alerte
    TEST_ASSERT_FALSE(a.hasPending());
}

// --- Aggravation LOW -> CRITICAL : seconde alerte ---------------------------
void test_native_escalates_to_critical() {
    BatteryAlertLogic a(kT);
    feed(a, 3.35f, 3);
    a.markNotified();
    feed(a, 3.18f, 3);
    TEST_ASSERT_EQUAL_UINT8(LEVEL_CRITICAL, a.observed());
    TEST_ASSERT_TRUE(a.hasPending());
    a.markNotified();
    TEST_ASSERT_FALSE(a.hasPending());
}

// --- Chute directe sous 3,20 V : une seule alerte CRITICAL, pas deux --------
void test_native_direct_critical() {
    BatteryAlertLogic a(kT);
    feed(a, 3.15f, 3);
    TEST_ASSERT_EQUAL_UINT8(LEVEL_CRITICAL, a.observed());
    a.markNotified();
    TEST_ASSERT_FALSE(a.hasPending());
}

// --- Hystérésis : remonter à 3,60 V ne réarme pas, 3,80 V oui ----------------
void test_native_rearm_hysteresis() {
    BatteryAlertLogic a(kT);
    feed(a, 3.35f, 3);
    a.markNotified();
    feed(a, 3.60f, 10);  // repos après émission : l'accu « remonte » un peu
    TEST_ASSERT_EQUAL_UINT8(LEVEL_LOW, a.observed());
    TEST_ASSERT_FALSE(a.hasPending());
    feed(a, 4.15f, 3);   // accu remplacé
    TEST_ASSERT_EQUAL_UINT8(LEVEL_OK, a.observed());
    TEST_ASSERT_TRUE(a.hasPending());  // confirmation « accu remplacé »
    a.markNotified();
    TEST_ASSERT_FALSE(a.hasPending());
}

// --- Envoi raté : l'alerte reste due jusqu'au succès -------------------------
void test_native_pending_until_sent() {
    BatteryAlertLogic a(kT);
    feed(a, 3.30f, 3);
    feed(a, 3.30f, 5);   // aucun markNotified (morfNotify injoignable)
    TEST_ASSERT_TRUE(a.hasPending());
}

// --- Redémarrage du hub : pas de renvoi de l'alerte déjà faite ---------------
void test_native_restore_after_reboot() {
    BatteryAlertLogic a(kT);
    a.restore(LEVEL_LOW);
    feed(a, 3.35f, 5);
    TEST_ASSERT_FALSE(a.hasPending());
    feed(a, 3.10f, 3);
    TEST_ASSERT_TRUE(a.hasPending());
}

// --- Valeur persistée corrompue : repli sur OK ------------------------------
void test_native_restore_garbage() {
    BatteryAlertLogic a(kT);
    a.restore(42);
    TEST_ASSERT_EQUAL_UINT8(LEVEL_OK, a.notified());
    TEST_ASSERT_FALSE(a.hasPending());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_healthy_no_alert);
    RUN_TEST(test_native_single_dip_ignored);
    RUN_TEST(test_native_low_after_confirm);
    RUN_TEST(test_native_escalates_to_critical);
    RUN_TEST(test_native_direct_critical);
    RUN_TEST(test_native_rearm_hysteresis);
    RUN_TEST(test_native_pending_until_sent);
    RUN_TEST(test_native_restore_after_reboot);
    RUN_TEST(test_native_restore_garbage);
    return UNITY_END();
}
