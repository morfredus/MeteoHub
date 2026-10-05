#pragma once
#include <string>

// Anneau de logs en RAM (consulte par /api/logs). 300 lignes (~30 Ko) : assez pour
// couvrir plusieurs heures d'un hub au repos, donc pour relire un incident apres
// coup (l'ancien anneau de 10 lignes etait ecrase en quelques secondes par un
// appairage). Chaque ligne est horodatee (heure murale si le NTP est la, sinon
// secondes depuis le boot) ; le meme texte part sur le port serie et en UDP.
#define LOG_BUFFER_SIZE 300

void addLog(const std::string& msg);
std::string getLog(int index);
int getLogCount();
void clearLogs();

#define LOG_DEBUG(msg) addLog(std::string("[DEBUG] ") + std::string(msg))
#define LOG_INFO(msg) addLog(std::string("[INFO] ") + std::string(msg))
#define LOG_WARNING(msg) addLog(std::string("[WARN] ") + std::string(msg))
#define LOG_ERROR(msg) addLog(std::string("[ERROR] ") + std::string(msg))
