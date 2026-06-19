# Structure du projet MeteoHub

## Fichiers principaux
- `main.cpp` : point d'entrée du programme, boucle principale.
- `config.h` : configuration utilisateur (paramètres modifiables).
- `board_config.h` : configuration matérielle (GPIO, etc).
- `secrets.h` : informations sensibles (WiFi, API).

## Exemple détaillé : main.cpp
```cpp
#include <Arduino.h>
#include "config.h"
#include "modules/sensors.h"

Sensors sensors;

void setup() {
    Serial.begin(115200);
}

void loop() {
    float temp = sensors.getTemperature();
    Serial.print("Température : ");
    Serial.println(temp);
    delay(1000);
}
```

## Organisation des dossiers
- `src/` : code source principal
- `include/` : fichiers d'en-tête partagés
- `lib/` : bibliothèques tierces
- `data/` : fichiers web embarqués
- `docs/` : documentation
