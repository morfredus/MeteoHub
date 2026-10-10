#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <MD5Builder.h>
#include "meteo_packet.h"

// ============================================================================
// SensorFwStore - firmware de la SONDE conserve sur le hub, pour sa mise a jour OTA
// ============================================================================
// Le hub ne flashe pas la sonde lui-meme : il garde le binaire sur LittleFS et le sert en
// HTTP (/sensor-fw.bin) ; quand la sonde declare une version differente (fw_version de ses
// trames), il lui envoie une OtaOffer (version, taille, md5) et elle vient le chercher sur
// le SoftAP « MH-NOW ». Un seul binaire a la fois. Meta = 3 lignes : version, taille, md5.
//
// Thread : le televersement tourne dans la tache AsyncTCP, la lecture (makeOffer) dans
// loop(). `_ready` est positionne EN DERNIER, apres ecriture du binaire ET de la meta :
// loop() ne voit jamais un binaire a moitie ecrit.
class SensorFwStore {
public:
    static constexpr const char* BIN_PATH  = "/sensor-fw.bin";
    static constexpr const char* META_PATH = "/sensor-fw.meta";

    // Relit la meta au demarrage. Un binaire dont la taille ne colle pas est ignore.
    void load() {
        _ready = false;
        File m = LittleFS.open(META_PATH, "r");
        if (!m) return;
        const String ver = m.readStringUntil('\n');
        const uint32_t size = (uint32_t)m.readStringUntil('\n').toInt();
        String md5 = m.readStringUntil('\n');
        m.close();
        md5.trim();
        File b = LittleFS.open(BIN_PATH, "r");
        const bool sizeOk = b && b.size() == size && size > 0;
        if (b) b.close();
        const uint32_t fw = encodeFwVersion(ver.c_str());
        if (!sizeOk || fw == 0 || md5.length() != 32) return;
        _fw = fw; _size = size;
        strlcpy(_md5, md5.c_str(), sizeof(_md5));
        strlcpy(_version, ver.c_str(), sizeof(_version));
        _ready = true;
    }

    bool present() const { return _ready; }
    const char* version() const { return _version; }
    uint32_t size() const { return _size; }
    const char* md5() const { return _md5; }

    // Offre a faire a une sonde qui declare `sensorFw` ; false si rien a proposer. Une sonde
    // qui ne declare aucune version (0 = firmware sans OTA) n'est pas sollicitee.
    bool makeOffer(uint32_t sensorFw, uint8_t nodeId, OtaOffer& out) const {
        if (!_ready || sensorFw == 0 || sensorFw == _fw) return false;
        memset(&out, 0, sizeof(out));
        out.node_id = nodeId;
        out.fw_version = _fw;
        out.size = _size;
        strlcpy(out.md5, _md5, sizeof(out.md5));
        sealOtaOffer(out);
        return true;
    }

    // --- Televersement (appele par la tache web, bloc par bloc) -------------------------
    bool beginUpload(const char* version) {
        clear();
        if (encodeFwVersion(version) == 0) return false;
        strlcpy(_version, version, sizeof(_version));
        _up = LittleFS.open(BIN_PATH, "w");
        _md5b.begin();
        _written = 0;
        return (bool)_up;
    }
    bool write(const uint8_t* data, size_t len) {
        if (!_up || _up.write(data, len) != len) return false;
        _md5b.add(const_cast<uint8_t*>(data), len);
        _written += len;
        return true;
    }
    bool endUpload() {
        if (!_up) return false;
        _up.close();
        if (_written == 0) { clear(); return false; }
        _md5b.calculate();
        strlcpy(_md5, _md5b.toString().c_str(), sizeof(_md5));
        File m = LittleFS.open(META_PATH, "w");
        if (!m) { clear(); return false; }
        m.printf("%s\n%u\n%s\n", _version, (unsigned)_written, _md5);
        m.close();
        _fw = encodeFwVersion(_version);
        _size = _written;
        _ready = true;
        return true;
    }
    void abortUpload() { if (_up) _up.close(); clear(); }

    void clear() {
        _ready = false;
        LittleFS.remove(BIN_PATH);
        LittleFS.remove(META_PATH);
    }

private:
    volatile bool _ready = false;
    uint32_t _fw = 0, _size = 0;
    char _version[16] = {};
    char _md5[33] = {};
    File _up;
    MD5Builder _md5b;
    size_t _written = 0;
};
