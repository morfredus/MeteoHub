# C++ Basics for MeteoHub

## Variables
```cpp
int temperature = 25; // Déclaration d'une variable entière
float humidity = 60.5; // Variable à virgule flottante
```

## Fonctions
```cpp
float calculateAverage(float a, float b) {
    return (a + b) / 2.0;
}

void displayValue(float value) {
    Serial.print("Valeur : ");
    Serial.println(value);
}
```

## Classes
```cpp
class Sensor {
public:
    Sensor(int pin) : pin_(pin) {}
    float read() {
        // Simule une lecture
        return 42.0;
    }
private:
    int pin_;
};

Sensor tempSensor(4);
float t = tempSensor.read();
```

## Structures de contrôle
```cpp
if (temperature > 30) {
    // Action si la température est élevée
} else {
    // Action sinon
}

for (int i = 0; i < 10; i++) {
    // Boucle 10 fois
}
```

## Tableaux
```cpp
float mesures[5] = {22.1, 23.5, 24.0, 25.2, 26.3};
for (int i = 0; i < 5; i++) {
    Serial.println(mesures[i]);
}
```

## Utilisation d'une bibliothèque
```cpp
#include <Wire.h>
#include "sensors.h"

Sensors sensors;
float temp = sensors.getTemperature();
```
