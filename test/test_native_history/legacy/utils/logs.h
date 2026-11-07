#pragma once
#include <string>
// Doublure des logs : les messages sont captures par les tests.
void addLog(const std::string& msg);
#define LOG_DEBUG(msg) addLog(std::string("[DEBUG] ") + std::string(msg))
#define LOG_INFO(msg) addLog(std::string("[INFO] ") + std::string(msg))
#define LOG_WARNING(msg) addLog(std::string("[WARN] ") + std::string(msg))
#define LOG_ERROR(msg) addLog(std::string("[ERROR] ") + std::string(msg))
