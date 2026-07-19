#pragma once
#include <string>

// Diffusion des logs par UDP (monitoring réseau sans câble série).
//
// - udpLogBegin() : à appeler une fois après l'initialisation du WiFi. Installe la
//   redirection des logs du cœur ESP-IDF/Arduino (WiFi, I2C, watchdog…) et démarre
//   une tâche d'émission dédiée.
// - udpLogSend()  : diffuse une ligne applicative (appelé par addLog()).
//
// L'émission réelle se fait dans une tâche séparée, alimentée par une file : le
// contexte d'où provient un log (y compris la pile réseau) n'émet jamais lui-même
// de paquet UDP, ce qui évite tout risque de réentrance/blocage.
void udpLogBegin();
void udpLogSend(const std::string& line);
