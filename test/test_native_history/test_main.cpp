// Tests de l'historique des mesures (stockage SD/LittleFS, requetes, export).
// Voir legacy_ref.h pour le code de reference et test_characterization.cpp /
// test_equivalence.cpp pour ce qui est prouve.
#include <unity.h>
#include <cstdlib>
#include <ctime>
#include "test_util.h"

void run_characterization();
void run_equivalence();
void run_equiv_store();
void run_equiv_ring();
void run_equiv_query();
void run_equiv_jobs();

void setUp() {}
void tearDown() {}

int main(int, char**) {
    // Fuseau fixe : les chemins de fichiers dependent de la date locale.
#ifdef _WIN32
    _putenv("TZ=UTC0");
#else
    setenv("TZ", "UTC0", 1);
#endif
    tzset();
    UNITY_BEGIN();
    run_characterization();
    run_equivalence();
    run_equiv_store();
    run_equiv_ring();
    run_equiv_query();
    run_equiv_jobs();
    return UNITY_END();
}
