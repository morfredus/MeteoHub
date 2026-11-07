#pragma once
#include <cstddef>
// Doublure : compte les cessions de la main (au lieu de rearmer le watchdog) pour
// verifier que la refonte n'en retire aucune.
extern unsigned long g_legacyYieldCalls;
inline void cooperativeYieldEvery(std::size_t iteration, std::size_t period) {
    if (period != 0 && (iteration % period) == 0) g_legacyYieldCalls++;
}
#define COOPERATIVE_YIELD_EVERY(iteration, period) cooperativeYieldEvery((iteration), (period))
