#include "udp_logger.h"
#include "config.h"

#if defined(UDP_LOG_ENABLED) && UDP_LOG_ENABLED

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_log.h>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#ifndef UDP_LOG_PORT
#define UDP_LOG_PORT 5005
#endif
#ifndef UDP_LOG_HOST
#define UDP_LOG_HOST "255.255.255.255"
#endif

#define UDP_LOG_LINE_MAX   200   // taille max d'une ligne diffusée
#define UDP_LOG_QUEUE_LEN  40    // profondeur de la file (les surplus sont ignorés)

struct LogItem { char text[UDP_LOG_LINE_MAX]; };

static WiFiUDP        s_udp;
static bool           s_started    = false;
static bool           s_broadcast  = false;
static IPAddress      s_target;
static vprintf_like_t s_prevVprintf = nullptr;
static QueueHandle_t  s_queue      = nullptr;

// Destination : IP fixe configurée, ou broadcast dirigé sur le sous-réseau courant.
static IPAddress computeDest() {
    if (!s_broadcast) return s_target;
    IPAddress ip = WiFi.localIP();
    IPAddress mask = WiFi.subnetMask();
    IPAddress dest;
    for (int i = 0; i < 4; ++i) dest[i] = (ip[i] & mask[i]) | (~mask[i] & 0xFF);
    return dest;
}

// Empile une ligne dans la file (non bloquant, sûr depuis n'importe quel contexte,
// y compris ISR). L'émission UDP est faite plus tard par la tâche dédiée.
static void enqueueLine(const char* data, size_t len) {
    if (!s_queue) return;
    LogItem item;
    const size_t n = len < (UDP_LOG_LINE_MAX - 1) ? len : (UDP_LOG_LINE_MAX - 1);
    memcpy(item.text, data, n);
    item.text[n] = '\0';

    if (xPortInIsrContext()) {
        BaseType_t hpw = pdFALSE;
        xQueueSendFromISR(s_queue, &item, &hpw);
        if (hpw) portYIELD_FROM_ISR();
    } else {
        xQueueSend(s_queue, &item, 0); // 0 tick : on ignore le log si la file est pleine
    }
}

// Tâche d'émission : seule à toucher le socket UDP -> aucun accès concurrent.
static void udpSenderTask(void*) {
    LogItem item;
    for (;;) {
        if (xQueueReceive(s_queue, &item, portMAX_DELAY) == pdTRUE) {
            if (WiFi.status() != WL_CONNECTED) continue;
            s_udp.beginPacket(computeDest(), UDP_LOG_PORT);
            s_udp.write(reinterpret_cast<const uint8_t*>(item.text), strlen(item.text));
            s_udp.endPacket();
        }
    }
}

// Redirection des logs du cœur ESP-IDF/Arduino : conserve la sortie série d'origine
// puis met la ligne en file pour diffusion UDP.
static int udpLogVprintf(const char* fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    const int n = s_prevVprintf ? s_prevVprintf(fmt, args) : vprintf(fmt, args); // série
    char buf[UDP_LOG_LINE_MAX];
    const int m = vsnprintf(buf, sizeof(buf), fmt, copy);
    va_end(copy);
    if (m > 0) enqueueLine(buf, (size_t)m < sizeof(buf) ? (size_t)m : sizeof(buf) - 1);
    return n;
}

void udpLogBegin() {
    if (s_started) return;

    s_broadcast = (strcmp(UDP_LOG_HOST, "255.255.255.255") == 0);
    if (!s_broadcast) s_target.fromString(UDP_LOG_HOST);

    s_queue = xQueueCreate(UDP_LOG_QUEUE_LEN, sizeof(LogItem));
    if (!s_queue) return;

    s_udp.begin(0); // port local éphémère (émission uniquement)
    xTaskCreatePinnedToCore(udpSenderTask, "udpLog", 4096, nullptr, 1, nullptr, 0);

    s_started = true;
    s_prevVprintf = esp_log_set_vprintf(&udpLogVprintf); // capture aussi les logs cœur
}

void udpLogSend(const std::string& line) {
    if (!s_started) return;
    std::string out = line;
    out.push_back('\n'); // une ligne par log
    enqueueLine(out.c_str(), out.size());
}

#else // UDP_LOG_ENABLED == 0

void udpLogBegin() {}
void udpLogSend(const std::string&) {}

#endif
