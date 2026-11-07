#pragma once
// Format des fichiers journaliers .bin de l'historique (IN et OUT) : en-tete, enregistrements
// v1 (16 octets) et v2 (20 octets, avec marques), lecture par index, parcours sequentiel,
// ajout avec reconnaissance de l'alignement.
//
// Aucune dependance Arduino : les fonctions qui touchent un fichier sont des modeles sur le
// type de fichier F (File Arduino sur le firmware, faux fichier memoire dans les tests). F
// doit offrir size(), seek(pos), read(buf, n), write(buf, n), flush() et close().
//
// CE FORMAT EST CELUI DES CARTES SD EN SERVICE : ne rien changer a la disposition des
// octets. Un fichier ecrit ici doit rester lisible par les firmwares precedents (retour
// arriere possible) et inversement.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "history_platform.h"

namespace mhhist {

// Une mesure de 16 octets : horodatage Unix, temperature, humidite, pression.
struct __attribute__((packed)) BinRecord {
    uint32_t ts;
    float t;
    float h;
    float p;
};

// Format v2 (1.54.0) : les 16 octets v1, suivis d'un champ de MARQUES (bits de
// cold_boot_rule.h). Un lecteur v1 n'exploite que les 16 premiers octets et avance selon
// recordSize : il lit un fichier v2 sans le comprendre entierement, jamais de travers.
struct __attribute__((packed)) BinRecordV2 {
    BinRecord base;
    uint32_t flags;
};

constexpr uint16_t kFormatV1 = 1;
constexpr uint16_t kFormatV2 = 2;  // enregistrements BinRecordV2
constexpr uint16_t kSensorTemp = 0x0001;
constexpr uint16_t kSensorHum = 0x0002;
constexpr uint16_t kSensorPres = 0x0004;

// En-tete present au debut de chaque fichier .bin : magic + version + tailles, pour que la
// lecture s'appuie sur recordSize (de futurs enregistrements plus grands ne cassent rien).
struct __attribute__((packed)) FileHeader {
    char magic[4];           // "MTHB"
    uint16_t version;        // format du fichier
    uint16_t headerSize;     // taille de l'en-tete
    uint16_t recordSize;     // taille d'un enregistrement
    uint16_t sensorFlags;    // capteurs presents (bits)
    uint32_t recordCount;    // nombre d'enregistrements
    uint64_t firstTimestamp; // premier releve
    uint64_t lastTimestamp;  // dernier releve
};

// Garde-fous de disposition : la taille de ces structures EST le format disque.
static_assert(sizeof(BinRecord) == 16, "BinRecord : 16 octets (format v1)");
static_assert(sizeof(BinRecordV2) == 20, "BinRecordV2 : 20 octets (format v2)");
static_assert(sizeof(FileHeader) == 32, "FileHeader : 32 octets");

inline bool headerMagicOk(const FileHeader& h) {
    return h.magic[0] == 'M' && h.magic[1] == 'T' && h.magic[2] == 'H' && h.magic[3] == 'B';
}

// En-tete d'un fichier neuf (compteurs a zero). withFlags : enregistrements v2.
FileHeader makeHeader(bool withFlags = false);

// Disposition d'un .bin ouvert : ou commencent les enregistrements, leur taille, et combien
// il y en a. Couvre les fichiers avec en-tete ET les anciens fichiers sans en-tete (v1.4.0 :
// enregistrements des l'offset 0).
struct BinLayout {
    uint32_t dataOffset;
    uint16_t recordSize;
    size_t nrec;
};

// Analyse un fichier ouvert. hdrOut / hasHeader (optionnels) rendent l'en-tete lu et
// disent s'il etait valide : l'ecriture IN en a besoin pour tenir ses compteurs.
template <typename F>
BinLayout probeBin(F& f, FileHeader* hdrOut = nullptr, bool* hasHeader = nullptr) {
    BinLayout L;
    L.dataOffset = 0;
    L.recordSize = sizeof(BinRecord);
    L.nrec = 0;
    if (hasHeader) *hasHeader = false;
    const size_t fsize = f.size();
    if (fsize >= sizeof(FileHeader)) {
        FileHeader h;
        f.seek(0);
        if (f.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) == (int)sizeof(h) && headerMagicOk(h)) {
            if (hdrOut) *hdrOut = h;
            if (hasHeader) *hasHeader = true;
            L.dataOffset = h.headerSize ? h.headerSize : sizeof(FileHeader);
            L.recordSize = h.recordSize >= sizeof(BinRecord) ? h.recordSize : sizeof(BinRecord);
            const size_t avail = fsize > L.dataOffset ? (fsize - L.dataOffset) : 0;
            L.nrec = avail / L.recordSize; // recalcule depuis la taille (robuste)
            return L;
        }
    }
    L.nrec = fsize / sizeof(BinRecord); // ancien format sans en-tete
    return L;
}

// Lit l'enregistrement d'index idx (seuls les 16 premiers octets : ts/t/h/p ; le pas de
// deplacement suit recordSize, d'ou la compatibilite avec de futurs enregistrements plus grands).
template <typename F>
bool readBinRecordAt(F& f, const BinLayout& L, size_t idx, BinRecord& br) {
    if (!f.seek(L.dataOffset + static_cast<uint32_t>(idx) * L.recordSize)) return false;
    return f.read(reinterpret_cast<uint8_t*>(&br), sizeof(BinRecord)) == (int)sizeof(BinRecord);
}

// Parcours SEQUENTIEL des enregistrements [startIdx, nrec) d'un .bin, par blocs d'environ
// 1 Ko (un seul seek initial, aucun seek par mesure) : la lecture SD mesure par mesure est
// nettement plus lente. Vaut pour toute taille d'enregistrement (v1 16 octets, v2 20 octets).
// Le callback recoit l'enregistrement et ses MARQUES (0 en v1), et renvoie false pour arreter
// (p. ex. horodatage au-dela de la plage).
template <typename F, typename Fn>
void forEachBinRecordFrom(F& f, const BinLayout& L, size_t startIdx, Fn&& cb) {
    if (startIdx >= L.nrec || L.recordSize < sizeof(BinRecord)) return;
    if (!f.seek(L.dataOffset + static_cast<uint32_t>(startIdx) * L.recordSize)) return;

    uint8_t buf[1280];
    const size_t perChunk = sizeof(buf) / L.recordSize;
    const bool hasFlags = L.recordSize >= sizeof(BinRecordV2);
    size_t remaining = L.nrec - startIdx;
    size_t processed = 0;
    while (remaining > 0) {
        const size_t want = remaining < perChunk ? remaining : perChunk;
        const int got = f.read(buf, want * L.recordSize);
        if (got <= 0) break;
        const size_t nread = static_cast<size_t>(got) / L.recordSize;
        for (size_t k = 0; k < nread; ++k) {
            const uint8_t* rec = buf + k * L.recordSize;
            BinRecord br;
            memcpy(&br, rec, sizeof(br));
            uint32_t flags = 0;
            if (hasFlags) memcpy(&flags, rec + sizeof(BinRecord), sizeof(flags));
            if (!cb(br, flags)) return;
        }
        MH_YIELD_EVERY(processed, 256);
        processed += nread;
        if (nread < want) break; // fin de fichier
        remaining -= nread;
    }
}

// Copie les enregistrements NON marques d'un .bin dans [from, to], TRIES par horodatage.
// Cote OUT, une mesure retransmise est ajoutee en fin de fichier avec son heure d'origine :
// le fichier n'est donc pas chronologique (l'index d'ajout, lui, reste le curseur de
// morfAnalytics). Les graphiques et le CSV du hub exigent un ordre temporel.
template <typename F>
void collectSortedDay(F& f, const BinLayout& L, long from_l, long to_l, std::vector<BinRecord>& out) {
    forEachBinRecordFrom(f, L, 0, [&](const BinRecord& rec, uint32_t flags) -> bool {
        const long ts = static_cast<long>(rec.ts);
        if (!flags && ts >= from_l && ts <= to_l) out.push_back(rec);
        return true;
    });
    std::stable_sort(out.begin(), out.end(),
                     [](const BinRecord& a, const BinRecord& b) { return a.ts < b.ts; });
}

// Octets de bourrage a ecrire avant le prochain enregistrement pour revenir sur une
// frontiere d'enregistrement : une ecriture interrompue (coupure, carte retiree) laisse un
// enregistrement partiel, et tout ce qui suivrait serait lu decale de quelques octets
// (horodatages et mesures faux). Le bourrage referme l'enregistrement partiel (une ligne
// douteuse isolee) au lieu de decaler tous les suivants.
inline size_t padToRecordBoundary(size_t fileSize, size_t dataOffset, size_t recordSize) {
    if (recordSize == 0 || fileSize <= dataOffset) return 0;
    const size_t rem = (fileSize - dataOffset) % recordSize;
    return rem ? recordSize - rem : 0;
}

// Resultat d'un ajout. padSeen : octets de bourrage necessaires (fichier desaligne) ;
// padWritten : octets reellement ecrits (0 si le desalignement depasse le garde-fou de 32
// octets, auquel cas on n'ecrit rien plutot que de bourrer sur une taille inattendue).
struct AppendOutcome {
    bool ok = false;
    size_t padSeen = 0;
    size_t padWritten = 0;
};

// Ajoute un enregistrement a un fichier ouvert EN AJOUT, en UNE seule ecriture dont la taille
// est verifiee : [en-tete si fichier neuf][bourrage de realignement][enregistrement]. Les
// octets deja presents ne sont jamais modifies. Le fichier est vide (flush) puis ferme.
//  - freshHeader : en-tete a ecrire (fichier neuf) ; nullptr pour un fichier existant.
//  - dataOffset / recordSize : disposition du fichier existant, pour le realignement.
template <typename F>
AppendOutcome appendRecord(F& f, const FileHeader* freshHeader, size_t dataOffset, size_t recordSize,
                           const void* rec, size_t recLen) {
    AppendOutcome out;
    uint8_t buf[96];
    size_t len = 0;
    if (freshHeader) {
        memcpy(buf, freshHeader, sizeof(FileHeader));
        len = sizeof(FileHeader);
    } else {
        out.padSeen = padToRecordBoundary(f.size(), dataOffset, recordSize);
        if (out.padSeen > 0 && out.padSeen < 32) {
            memset(buf, 0, out.padSeen);
            len = out.padWritten = out.padSeen;
        }
    }
    memcpy(buf + len, rec, recLen);
    len += recLen;
    out.ok = f.write(buf, len) == len;
    f.flush();
    f.close();
    return out;
}

} // namespace mhhist
