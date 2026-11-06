#include "system_info.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <SD.h>
#include <WiFi.h>
#include "config.h"
#include "managers/sd_manager.h"


std::string getSystemInfoJson(SdManager* sd) {
    DynamicJsonDocument doc(1024);

    doc["project_name"] = PROJECT_NAME;
    doc["project_version"] = PROJECT_VERSION;
    doc["uptime_s"] = millis() / 1000;
    doc["free_heap_b"] = ESP.getFreeHeap();
    doc["chip_model"] = ESP.getChipModel();
    doc["chip_revision"] = ESP.getChipRevision();
    doc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();

    if (psramFound()) {
        doc["free_psram_b"] = ESP.getFreePsram();
        doc["total_psram_b"] = ESP.getPsramSize();
    }

    doc["wifi_rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
    doc["wifi_ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "disconnected";
    doc["ip_address"] = WiFi.localIP().toString();

    JsonObject fs_littlefs = doc.createNestedObject("littlefs");
    fs_littlefs["total_b"] = LittleFS.totalBytes();
    fs_littlefs["used_b"] = LittleFS.usedBytes();

    if (sd && sd->isAvailable()) {
        JsonObject fs_sd = doc.createNestedObject("sd_card");
        fs_sd["total_b"] = SD.totalBytes();
        // Pas de used_b : SD.usedBytes() fait un f_getfree, qui parcourt toute la FAT quand
        // l'info de la carte n'est pas fiable. Sur 16 Go en FAT32 cela depasse 5 s dans la
        // tache async_tcp (footer.js appelle /api/system a CHAQUE page) : TASK_WDT.
        // Personne ne lit ce champ ; sur les 34 Mo de l'ancienne partition le scan etait instantane.
        fs_sd["size_mb"] = (uint32_t)(SD.cardSize() / (1024 * 1024));
    } else {
        doc["sd_card"] = "not_mounted";
    }

    std::string output;
    serializeJson(doc, output);
    return output;
}