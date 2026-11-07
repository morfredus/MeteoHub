#pragma once
// Doublure de SdManager pour le code de reference : deux interrupteurs de test.
class SdManager {
public:
    bool available = true;
    bool mounted = true;
    bool isAvailable() { return available; }
    bool ensureMounted() { return mounted; }
};
