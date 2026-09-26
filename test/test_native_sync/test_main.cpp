// Tests unitaires (hote) de la logique de synchronisation cote hub :
// SyncTracker (accuse cumulatif, detection de trous, dedup, anti-blocage) et
// TimeAnchor (reconstruction de l'heure de mesure). Sans materiel ni radio.
#include <unity.h>
#include "sync/sync_tracker.h"
#include "sync/time_anchor.h"

using namespace mhsync;

void setUp() {}
void tearDown() {}

// --- Reception dans l'ordre : l'accuse suit ---------------------------------
void test_native_inorder_ack_advances() {
    SyncTracker t;
    for (uint32_t s = 1; s <= 5; s++) TEST_ASSERT_TRUE(t.markReceived(s));
    TEST_ASSERT_EQUAL_UINT32(5, t.ackContiguous());
    TEST_ASSERT_FALSE(t.hasGap());
    TEST_ASSERT_EQUAL_UINT32(0, t.firstMissing());
}

// --- Trou : l'accuse s'arrete avant, le trou est detecte --------------------
void test_native_gap_detected() {
    SyncTracker t;
    t.markReceived(1); t.markReceived(2); t.markReceived(3);
    t.markReceived(5); t.markReceived(6); // 4 manque
    TEST_ASSERT_EQUAL_UINT32(3, t.ackContiguous());
    TEST_ASSERT_TRUE(t.hasGap());
    TEST_ASSERT_EQUAL_UINT32(4, t.firstMissing());
    // La retransmission de 4 comble le trou : l'accuse saute a 6.
    TEST_ASSERT_TRUE(t.markReceived(4));
    TEST_ASSERT_EQUAL_UINT32(6, t.ackContiguous());
    TEST_ASSERT_FALSE(t.hasGap());
}

// --- Dedup : un seq deja connu n'est pas « nouveau » ------------------------
void test_native_dedup_idempotent() {
    SyncTracker t;
    TEST_ASSERT_TRUE(t.markReceived(10));  // hors ordre, cree un trou 1..9
    TEST_ASSERT_FALSE(t.markReceived(10)); // doublon en avance
    t.noteSensorOldest(10);                // 1..9 jamais existes cote sonde
    TEST_ASSERT_EQUAL_UINT32(10, t.ackContiguous());
    TEST_ASSERT_FALSE(t.markReceived(10)); // doublon deja couvert par l'accuse
    TEST_ASSERT_FALSE(t.markReceived(5));  // vieux doublon
}

// --- Anti-blocage : une mesure perdue (buffer plein) ne fige pas l'accuse ---
void test_native_unrecoverable_gap_advances() {
    SyncTracker t;
    t.markReceived(100); t.markReceived(101); t.markReceived(102);
    // Trou 1..99 (jamais recus). La sonde n'a plus que >= 98 en buffer.
    TEST_ASSERT_EQUAL_UINT32(0, t.ackContiguous());
    t.noteSensorOldest(98); // le hub apprend que < 98 est irrecuperable
    // 98,99 restent reclamables ; l'accuse avance a 97.
    TEST_ASSERT_EQUAL_UINT32(97, t.ackContiguous());
    TEST_ASSERT_EQUAL_UINT32(98, t.firstMissing());
    // Puis la sonde retransmet 98 et 99 -> accuse saute a 102.
    t.markReceived(98); t.markReceived(99);
    TEST_ASSERT_EQUAL_UINT32(102, t.ackContiguous());
    TEST_ASSERT_FALSE(t.hasGap());
}

// --- Persistance : l'etat survit a un reboot du hub -------------------------
void test_native_state_roundtrip() {
    SyncTracker a;
    a.markReceived(1); a.markReceived(2); a.markReceived(4); a.markReceived(5);
    auto st = a.exportState();
    SyncTracker b;
    b.importState(st);
    TEST_ASSERT_EQUAL_UINT32(2, b.ackContiguous());
    TEST_ASSERT_EQUAL_UINT32(3, b.firstMissing());
    // b comble le trou comme si de rien n'etait.
    TEST_ASSERT_TRUE(b.markReceived(3));
    TEST_ASSERT_EQUAL_UINT32(5, b.ackContiguous());
}

// --- TimeAnchor : une retransmission tardive garde son heure de mesure ------
void test_native_time_reconstruction() {
    TimeAnchor anchor;
    TEST_ASSERT_FALSE(anchor.has());

    // Trame LIVE : sensor_ts=10000 recue a l'heure reelle 1'700'000'000.
    anchor.updateFromLive(10000, 1700000000LL);
    TEST_ASSERT_TRUE(anchor.has());

    // Reconstruction d'une mesure prise 100 min plus tot (sensor_ts=4000),
    // retransmise maintenant : 6000 s = 100 min avant l'ancre.
    int64_t r = anchor.reconstruct(4000);
    TEST_ASSERT_EQUAL_INT64(1700000000LL - 6000LL, r);

    // Une mesure a l'ancre exacte retombe sur l'heure reelle de l'ancre.
    TEST_ASSERT_EQUAL_INT64(1700000000LL, anchor.reconstruct(10000));
}

// --- L'ancre se rafraichit : la trame live la plus recente fait foi ---------
void test_native_anchor_refreshes() {
    TimeAnchor anchor;
    anchor.updateFromLive(10000, 1700000000LL);
    anchor.updateFromLive(10300, 1700000300LL); // 5 min plus tard
    // Une mesure a sensor_ts=10300 = maintenant.
    TEST_ASSERT_EQUAL_INT64(1700000300LL, anchor.reconstruct(10300));
    // Et la precedente (10000) = 300 s avant.
    TEST_ASSERT_EQUAL_INT64(1700000000LL, anchor.reconstruct(10000));
}

// --- Appairage : le nouveau hub reprend apres le dernier seq de l'ancien ------
// (MeteoSyncService::adoptBaseline s'appuie sur noteSensorOldest(base + 1)).
void test_native_pairing_baseline() {
    SyncTracker t;                       // hub neuf : n'a jamais vu cette sonde
    t.noteSensorOldest(1000 + 1);        // base_seq = 1000, deja livre a l'ancien hub
    TEST_ASSERT_EQUAL_UINT32(1000, t.ackContiguous());
    TEST_ASSERT_FALSE(t.hasGap());       // ne reclame PAS l'historique deja livre
    TEST_ASSERT_TRUE(t.markReceived(1003)); // live courant, 1001-1002 en attente
    TEST_ASSERT_EQUAL_UINT32(1001, t.firstMissing()); // seules les attentes sont reclamees
    t.markReceived(1001); t.markReceived(1002);
    TEST_ASSERT_EQUAL_UINT32(1003, t.ackContiguous());
    // Re-appairer le MEME hub avec une base plus ancienne ne fait rien oublier.
    t.noteSensorOldest(500 + 1);
    TEST_ASSERT_EQUAL_UINT32(1003, t.ackContiguous());
}

// --- Sonde repartie de seq=1 : detectee, pas prise pour des doublons ---------
void test_native_counter_restart_detected() {
    SyncTracker t;
    for (uint32_t s = 1; s <= 945; s++) t.markReceived(s);
    TEST_ASSERT_FALSE(t.isCounterRestart(946));  // live suivant : normal
    TEST_ASSERT_FALSE(t.isCounterRestart(945));  // re-emission du meme live
    TEST_ASSERT_TRUE(t.isCounterRestart(3));     // NVS effacee : seq repart a 1
    TEST_ASSERT_FALSE(t.isCounterRestart(0));
    SyncTracker fresh;                            // hub neuf : jamais de faux positif
    TEST_ASSERT_FALSE(fresh.isCounterRestart(3));
    // Seq en avance (trou) : le max vu est pris en compte, pas seulement l'accuse.
    SyncTracker gap;
    gap.markReceived(1); gap.markReceived(10);
    TEST_ASSERT_EQUAL_UINT32(10, gap.maxSeen());
    TEST_ASSERT_TRUE(gap.isCounterRestart(5));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_inorder_ack_advances);
    RUN_TEST(test_native_gap_detected);
    RUN_TEST(test_native_dedup_idempotent);
    RUN_TEST(test_native_unrecoverable_gap_advances);
    RUN_TEST(test_native_state_roundtrip);
    RUN_TEST(test_native_time_reconstruction);
    RUN_TEST(test_native_anchor_refreshes);
    RUN_TEST(test_native_pairing_baseline);
    RUN_TEST(test_native_counter_restart_detected);
    return UNITY_END();
}
