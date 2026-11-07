#pragma once
// Pont entre le code portable de l'historique (src/history/) et la plateforme.
//
// Sur le firmware (ARDUINO defini), les cessions de la main et les logs passent par les
// outils habituels du projet : l'ordre et la frequence des yields restent ceux du code
// d'origine (ils rearment le watchdog de la tache async_tcp, ne JAMAIS en retirer).
// Sur l'hote (tests natifs), les memes macros comptent les cessions et capturent les logs :
// un test peut ainsi verifier qu'une boucle longue cede toujours la main.
#include <cstddef>
#include <string>

#ifdef ARDUINO
#include "../utils/cooperative_yield.h"
#include "../utils/logs.h"
#define MH_YIELD_EVERY(iteration, period) COOPERATIVE_YIELD_EVERY((iteration), (period))
#define MH_LOG_INFO(msg) LOG_INFO(msg)
#define MH_LOG_WARNING(msg) LOG_WARNING(msg)
#define MH_LOG_ERROR(msg) LOG_ERROR(msg)
#else
#include <vector>
namespace mhhist {
inline unsigned long& testYieldCalls() {
    static unsigned long n = 0;
    return n;
}
inline std::vector<std::string>& testLogLines() {
    static std::vector<std::string> lines;
    return lines;
}
inline void countYield(std::size_t iteration, std::size_t period) {
    if (period != 0 && (iteration % period) == 0) testYieldCalls()++;
}
}  // namespace mhhist
#define MH_YIELD_EVERY(iteration, period) mhhist::countYield((iteration), (period))
#define MH_LOG_INFO(msg) mhhist::testLogLines().push_back(std::string("[INFO] ") + std::string(msg))
#define MH_LOG_WARNING(msg) mhhist::testLogLines().push_back(std::string("[WARN] ") + std::string(msg))
#define MH_LOG_ERROR(msg) mhhist::testLogLines().push_back(std::string("[ERROR] ") + std::string(msg))
#endif
