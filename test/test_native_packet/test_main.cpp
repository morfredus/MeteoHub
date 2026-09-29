// Tests unitaires (hôte) du décodage des trames de données : v3 (63 octets,
// sans version de firmware) et v4 (67 octets) acceptées pendant la transition,
// CRC vérifié à la bonne place. Sans matériel.
#include <unity.h>
#include <cstring>
#include "meteo_packet.h"

void setUp() {}
void tearDown() {}

static MeteoPacket sample(uint8_t version) {
    MeteoPacket p;
    memset(&p, 0, sizeof(p));
    p.magic[0] = METEO_PACKET_MAGIC_0;
    p.magic[1] = METEO_PACKET_MAGIC_1;
    p.protocol_version = version;
    p.sequence = 1915;
    p.temperature = 26.1f;
    p.pressure = 1014.1f;
    return p;
}

// --- v4 : la version du firmware traverse intacte -----------------------------
void test_native_v4_roundtrip() {
    MeteoPacket p = sample(METEO_DATA_VERSION);
    p.fw_version = encodeFwVersion("0.27.0");
    p.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&p), sizeof(p) - 2);
    TEST_ASSERT_TRUE(normalizeMeteoPacket(p, sizeof(MeteoPacket)));
    TEST_ASSERT_EQUAL_UINT32((0u << 16) | (27u << 8) | 0u, p.fw_version);
}

// --- v3 : trame d'une sonde pas encore mise à jour, toujours acceptée ---------
void test_native_v3_still_accepted() {
    // Octets tels qu'une sonde v3 les émet : 61 octets de corps + CRC.
    MeteoPacket v3 = sample(3);
    uint8_t wire[METEO_PACKET_V3_SIZE];
    memcpy(wire, &v3, METEO_PACKET_V3_SIZE - 2);
    const uint16_t crc = calculateCrc16(wire, METEO_PACKET_V3_SIZE - 2);
    wire[61] = (uint8_t)(crc & 0xFF);
    wire[62] = (uint8_t)(crc >> 8);
    // Copie dans la structure v4, comme le récepteur du hub.
    MeteoPacket rx;
    memset(&rx, 0, sizeof(rx));
    memcpy(&rx, wire, sizeof(wire));
    TEST_ASSERT_TRUE(normalizeMeteoPacket(rx, sizeof(wire)));
    TEST_ASSERT_EQUAL_UINT32(0, rx.fw_version);   // version inconnue
    TEST_ASSERT_EQUAL_UINT32(1915, rx.sequence);
    TEST_ASSERT_EQUAL_FLOAT(1014.1f, rx.pressure);
}

// --- Rejets : CRC faux, taille incohérente, version inconnue ------------------
void test_native_rejects() {
    MeteoPacket p = sample(METEO_DATA_VERSION);
    p.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&p), sizeof(p) - 2);
    MeteoPacket bad = p;
    bad.temperature = 99.0f;                                   // corrompue
    TEST_ASSERT_FALSE(normalizeMeteoPacket(bad, sizeof(MeteoPacket)));
    MeteoPacket shortLen = p;
    TEST_ASSERT_FALSE(normalizeMeteoPacket(shortLen, METEO_PACKET_V3_SIZE)); // v4 tronquée
    MeteoPacket v9 = sample(9);
    v9.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&v9), sizeof(v9) - 2);
    TEST_ASSERT_FALSE(normalizeMeteoPacket(v9, sizeof(MeteoPacket)));
}

// --- Encodage de version ------------------------------------------------------
void test_native_fw_version_encoding() {
    TEST_ASSERT_EQUAL_UINT32(0x001A00u, encodeFwVersion("0.26.0"));
    TEST_ASSERT_EQUAL_UINT32((1u << 16) | (54u << 8) | 1u, encodeFwVersion("1.54.1"));
    TEST_ASSERT_EQUAL_UINT32(0, encodeFwVersion("abc"));
    TEST_ASSERT_EQUAL_UINT32(0, encodeFwVersion("1.2"));
    TEST_ASSERT_EQUAL_UINT32(0, encodeFwVersion("1.300.0"));
    TEST_ASSERT_EQUAL_UINT32(0, encodeFwVersion(nullptr));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_native_v4_roundtrip);
    RUN_TEST(test_native_v3_still_accepted);
    RUN_TEST(test_native_rejects);
    RUN_TEST(test_native_fw_version_encoding);
    return UNITY_END();
}
