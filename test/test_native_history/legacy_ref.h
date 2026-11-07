#pragma once
// CODE DE REFERENCE : history_manager.{h,cpp} de la 1.62.0, copies telles quelles dans
// legacy/managers/, compilees sur l'hote dans le namespace `legacy` au-dessus du shim
// Arduino (shim/). Il sert d'etalon : la refonte doit produire exactement les memes
// octets sur la carte et les memes resultats de requete que ce code.
//
// Les en-tetes systeme et du projet sont inclus AVANT d'ouvrir le namespace (leur
// #pragma once / garde les rend ensuite inertes), pour qu'ils restent globaux.
#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstring>
#include <ctime>
#include <functional>
#include <inttypes.h>
#include <mutex>
#include <string>
#include <vector>

#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>
#include "temporal_filter.h"
#include "measure_range.h"
#include "config.h"
#include "legacy/utils/logs.h"
#include "legacy/utils/cooperative_yield.h"
#include "../../src/modules/meteo_context.h"

// Les membres prives du code de reference sont appeles directement par les tests
// (saveToSdBinary, compact..., etc.) : on les rend publics le temps de l'inclusion.
#define private public
namespace legacy {
#include "legacy/managers/history_manager.h"
}
#undef private

// Acces aux fonctions `static` du .cpp de reference (visibles seulement dans legacy_tu.cpp).
namespace legacy_probe {
struct Layout { uint32_t dataOffset; uint16_t recordSize; size_t nrec; };
Layout probe(const char* path);                       // probeBin sur un fichier du volume SD
bool readAt(const char* path, size_t idx, uint32_t& ts, float& t, float& h, float& p);
size_t pad(size_t fileSize, size_t dataOffset, size_t recordSize);
void header(bool withFlags, uint8_t out[32]);
bool upgrade(const char* path);                       // upgradeLegacyBin
bool writeReplacing(const char* path, const void* data, size_t len);
struct Rec { uint32_t ts; float t, h, p; };
// collectSortedDay sur un fichier : enregistrements NON marques de [from, to], tries.
std::vector<Rec> sortedDay(const char* path, long from, long to);
// forEachBinRecordFrom : (ts, flags) dans l'ordre de parcours.
std::vector<std::pair<uint32_t, uint32_t>> walk(const char* path, size_t startIdx);
bool tsPlausible(time_t ts);
}
