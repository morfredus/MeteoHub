// Tests unitaires (hote) de la serie batterie : lecture de ligne et agregation en
// seaux, sans materiel.
#include <unity.h>
#include "battery_series.h"

using namespace mhbat;

void setUp() {}
void tearDown() {}

void test_native_parse_line() {
    uint32_t ts; float v, p;
    TEST_ASSERT_TRUE(parseLine("1791203036,4.076,71\n", ts, v, p));
    TEST_ASSERT_EQUAL_UINT32(1791203036u, ts);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.076f, v);
    // En-tete, ligne vide, ligne tronquee : ignorees.
    TEST_ASSERT_FALSE(parseLine("ts,volts,pct\n", ts, v, p));
    TEST_ASSERT_FALSE(parseLine("\n", ts, v, p));
    TEST_ASSERT_FALSE(parseLine("1791203036,4.0", ts, v, p));
}

// 24 h : une mesure toutes les 5 min -> chaque mesure reste un point.
void test_native_short_span_keeps_every_measurement() {
    Aggregator a(1000000, 1000000 + 86400);
    for (uint32_t i = 0; i < 288; i++) a.add(1000000 + i * 300, 4.0f, 80);
    TEST_ASSERT_EQUAL_UINT32(300, a.bucketSeconds());
    TEST_ASSERT_EQUAL(288, a.points().size());
}

// 90 jours : ~26 000 mesures ramenees a <= 401 points, moyenne conservee.
void test_native_long_span_is_bounded_and_averaged() {
    const uint32_t from = 1000000, to = from + 90u * 86400u;
    Aggregator a(from, to);
    for (uint32_t t = from; t <= to; t += 300) a.add(t, 3.9f, 50);
    const auto pts = a.points();
    TEST_ASSERT_TRUE(pts.size() <= 401);
    TEST_ASSERT_TRUE(pts.size() > 300);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.9f, pts[10].volts);
}

// L'ordre d'arrivee est sans effet (retransmission ecrite apres des mesures recentes),
// et les mesures hors plage sont ecartees.
void test_native_order_independent_and_range_filtered() {
    Aggregator a(1000, 1000 + 3600);
    a.add(1000 + 1200, 4.0f, 90);
    a.add(1000 + 300, 4.2f, 100);   // arrivee tardive, plus ancienne
    a.add(5, 3.0f, 1);              // avant la plage
    a.add(1000 + 99999, 3.0f, 1);   // apres la plage
    const auto pts = a.points();
    TEST_ASSERT_EQUAL(2, pts.size());
    TEST_ASSERT_TRUE(pts[0].ts < pts[1].ts);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.2f, pts[0].volts);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_native_parse_line);
    RUN_TEST(test_native_short_span_keeps_every_measurement);
    RUN_TEST(test_native_long_span_is_bounded_and_averaged);
    RUN_TEST(test_native_order_independent_and_range_filtered);
    return UNITY_END();
}
