# Modules du Projet MeteoHub

## Exemple détaillé : Module capteur

```cpp
// src/modules/sensors.h
class Sensors {
public:
    Sensors();
    float getTemperature();
    float getHumidity();
};

// src/modules/sensors.cpp
#include "sensors.h"
Sensors::Sensors() {
    // Initialisation des capteurs
}
float Sensors::getTemperature() {
    // Code pour lire la température
    return 23.7;
}
float Sensors::getHumidity() {
    // Code pour lire l'humidité
    return 55.2;
}
```

## Exemple détaillé : Utilisation dans main.cpp

```cpp
#include "modules/sensors.h"
Sensors sensors;
void setup() {
    Serial.begin(115200);
}
void loop() {
    float t = sensors.getTemperature();
    float h = sensors.getHumidity();
    Serial.print("Température : ");
    Serial.println(t);
    Serial.print("Humidité : ");
    Serial.println(h);
    delay(2000);
}
```
