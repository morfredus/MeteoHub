#include "logs.h"
#include "udp_logger.h"
#include <Arduino.h>
#include <vector>
#include <string>

// Utilisation d'un vecteur pour stocker les logs en mémoire
static std::vector<std::string> logs;

void addLog(const std::string& msg) {
    // Si le buffer est plein, on retire le plus ancien
    if (logs.size() >= LOG_BUFFER_SIZE) {
        logs.erase(logs.begin());
    }
    logs.push_back(msg);

    Serial.println(msg.c_str()); // miroir sur le port série
    udpLogSend(msg);             // diffusion réseau (UDP), voir udp_logger
}

std::string getLog(int index) {
    if (index >= 0 && index < logs.size()) {
        return logs[index];
    }
    return "";
}

int getLogCount() {
    return logs.size();
}

void clearLogs() {
    logs.clear();
}

// Ajout d'une ligne vide pour forcer la modification
// (logs.cpp)