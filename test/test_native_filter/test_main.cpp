// Tests unitaires (hôte) du filtre temporel des statistiques : une vraie
// variation rapide, même forte, n'est JAMAIS écartée ; seul un aller-retour d'un
// point l'est. Sans matériel.
#include <unity.h>
#include <algorithm>
#include "temporal_filter.h"

using namespace mhfilter;
using Series = std::vector<std::pair<int64_t, float>>;

void setUp() {}
void tearDown() {}

// Série à cadence 5 min à partir de valeurs.
static Series mk(const std::vector<float>& v) {
    Series s;
    for (size_t i = 0; i < v.size(); ++i) s.push_back({(int64_t)(i * 300), v[i]});
    return s;
}

static float minOf(const std::vector<float>& v) { return *std::min_element(v.begin(), v.end()); }
static float maxOf(const std::vector<float>& v) { return *std::max_element(v.begin(), v.end()); }

// --- Journée calme puis vraie chute de 10 hPa (cas qui cassait le filtre MAD) --
void test_native_real_drop_after_calm_is_kept() {
    std::vector<float> v(240, 1015.0f);             // 20 h stables
    for (int i = 0; i < 48; ++i) v.push_back(1005.0f); // 4 h plus bas
    const auto k = keptValues(mk(v), 1.0f, true);
    TEST_ASSERT_EQUAL_UINT32(v.size(), k.size());
    TEST_ASSERT_EQUAL_FLOAT(1005.0f, minOf(k));
}

// --- Chute d'orage rapide : -6 hPa en 15 min, qui persiste --------------------
void test_native_storm_drop_is_kept() {
    const auto k = keptValues(mk({1012, 1012, 1010, 1008, 1006, 1006, 1006}), 1.0f, true);
    TEST_ASSERT_EQUAL_UINT32(7, k.size());
    TEST_ASSERT_EQUAL_FLOAT(1006.0f, minOf(k));
}

// --- Saut brusque d'une mesure à l'autre (+3 hPa, orage) qui dure 30 min ------
void test_native_sudden_step_is_kept() {
    const auto k = keptValues(mk({1010, 1010, 1013, 1013, 1013, 1013, 1013, 1013, 1010}), 1.0f, true);
    TEST_ASSERT_EQUAL_UINT32(9, k.size());
    TEST_ASSERT_EQUAL_FLOAT(1013.0f, maxOf(k));
}

// --- Aller-retour d'un seul point (artefact au réveil) : écarté ---------------
void test_native_single_spike_is_removed() {
    const auto k = keptValues(mk({1014.0f, 1014.1f, 1022.0f, 1014.1f, 1014.0f}), 1.0f, true);
    TEST_ASSERT_EQUAL_UINT32(4, k.size());
    TEST_ASSERT_TRUE(maxOf(k) < 1015.0f);
}

// --- Retransmission arrivée en retard : jugée à sa place dans le temps --------
void test_native_out_of_order_is_sorted() {
    Series s = mk({1012, 1011, 1010, 1009});
    std::swap(s[1], s[3]);   // ordre d'arrivée différent de l'ordre de mesure
    const auto k = keptValues(s, 1.0f, true);
    TEST_ASSERT_EQUAL_UINT32(4, k.size());   // rampe régulière : rien d'écarté
}

// --- Champ non mesuré (0) ignoré, sans fausser les voisins --------------------
void test_native_absent_values_ignored() {
    const auto k = keptValues(mk({1012, 0, 1012, 1011}), 1.0f, true);
    TEST_ASSERT_EQUAL_UINT32(3, k.size());
    TEST_ASSERT_EQUAL_FLOAT(1011.0f, minOf(k));
    // La température, elle, peut valoir 0 : conservée.
    TEST_ASSERT_EQUAL_UINT32(3, keptValues(mk({0.5f, 0.0f, -0.4f}), 1.0f, false).size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_real_drop_after_calm_is_kept);
    RUN_TEST(test_native_storm_drop_is_kept);
    RUN_TEST(test_native_sudden_step_is_kept);
    RUN_TEST(test_native_single_spike_is_removed);
    RUN_TEST(test_native_out_of_order_is_sorted);
    RUN_TEST(test_native_absent_values_ignored);
    return UNITY_END();
}
