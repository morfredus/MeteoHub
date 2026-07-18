#pragma once

#include <Arduino.h>
#include <cstddef>
#include <esp_task_wdt.h>

inline void cooperativeYieldEvery(std::size_t iteration, std::size_t period) {
    if (period == 0) {
        return;
    }

    if ((iteration % period) == 0) {
        // Réarme le watchdog de la tâche courante avant de céder la main. Sans cet
        // appel, une opération longue exécutée dans la tâche async_tcp (ex. lecture
        // de plusieurs fichiers CSV sur la carte SD lors d'une requête /api/history)
        // ne nourrit pas le task watchdog et provoque un abort/reboot. La valeur de
        // retour (ESP_ERR_NOT_FOUND si la tâche n'est pas surveillée) est ignorée.
        esp_task_wdt_reset();
        delay(0);
    }
}

#define COOPERATIVE_YIELD_EVERY(iteration, period) cooperativeYieldEvery((iteration), (period))
