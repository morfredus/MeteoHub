#include "logs.h"
#include "udp_logger.h"
#include <Arduino.h>
#include <deque>
#include <string>
#include <time.h>
#include <freertos/semphr.h>

// Anneau en memoire. Plusieurs taches ecrivent (boucle principale, evenements
// Wi-Fi, serveur web) : un mutex protege le conteneur, sinon un push concurrent
// pendant une lecture de /api/logs corrompt le deque.
static std::deque<std::string> logs;

static SemaphoreHandle_t logMutex() {
    static SemaphoreHandle_t m = xSemaphoreCreateMutex();   // init thread-safe (C++11)
    return m;
}

// Horodatage : heure murale des que le NTP a recale l'horloge, sinon uptime. L'uptime
// seul ne suffit pas a recouper un incident avec l'historique ; l'heure seule
// manquerait avant la synchro NTP, au boot.
static std::string stamp() {
    const time_t now = time(nullptr);
    char buf[16];
    if (now > 1600000000) {
        struct tm t;
        localtime_r(&now, &t);
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d ", t.tm_hour, t.tm_min, t.tm_sec);
    } else {
        snprintf(buf, sizeof(buf), "+%lus ", (unsigned long)(millis() / 1000UL));
    }
    return buf;
}

void addLog(const std::string& msg) {
    const std::string line = stamp() + msg;
    SemaphoreHandle_t m = logMutex();
    xSemaphoreTake(m, portMAX_DELAY);
    if (logs.size() >= LOG_BUFFER_SIZE) logs.pop_front();
    logs.push_back(line);
    xSemaphoreGive(m);

    Serial.println(line.c_str()); // miroir sur le port serie
    udpLogSend(line);             // diffusion reseau (UDP), voir udp_logger
}

std::string getLog(int index) {
    SemaphoreHandle_t m = logMutex();
    xSemaphoreTake(m, portMAX_DELAY);
    std::string out;
    if (index >= 0 && index < (int)logs.size()) out = logs[index];
    xSemaphoreGive(m);
    return out;
}

int getLogCount() {
    SemaphoreHandle_t m = logMutex();
    xSemaphoreTake(m, portMAX_DELAY);
    const int n = (int)logs.size();
    xSemaphoreGive(m);
    return n;
}

void clearLogs() {
    SemaphoreHandle_t m = logMutex();
    xSemaphoreTake(m, portMAX_DELAY);
    logs.clear();
    xSemaphoreGive(m);
}
