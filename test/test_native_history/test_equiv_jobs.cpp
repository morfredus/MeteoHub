// Equivalence des travaux de fond (effacement par lots, migration des anciens CSV, creation de
// l'arborescence) et de l'archive des previsions avec le code de reference.
#include <string>
#include "test_util.h"
#include "test_scenarios.h"
#include "history/history_jobs.h"
#include "history/forecast_archive.h"

static const time_t T0 = mkUtc(2026, 11, 5, 12, 0, 0);

// Meme arborescence des deux cotes : annees/mois/jours, plus des fichiers hors modele.
static void buildTree(MemVolume& v, int years, int months, int filesPerMonth) {
    v.dirs.insert("/history");
    for (const char* root : {"/history/indoor", "/history/outdoor"}) {
        v.dirs.insert(root);
        for (int y = 0; y < years; y++) {
            const std::string yp = std::string(root) + "/" + std::to_string(2025 + y);
            v.dirs.insert(yp);
            for (int m = 1; m <= months; m++) {
                char mp[96];
                snprintf(mp, sizeof(mp), "%s/%02d", yp.c_str(), m);
                v.dirs.insert(mp);
                for (int d = 1; d <= filesPerMonth; d++) {
                    char fp[128];
                    snprintf(fp, sizeof(fp), "%s/2025-%02d-%02d.bin", mp, m, d);
                    v.files[fp] = std::vector<uint8_t>(40, (uint8_t)d);
                }
            }
        }
    }
    v.files["/history/forecast/x.json"] = {1};
    v.dirs.insert("/history/forecast");
    v.files["/history/2026-11-04.csv"] = {1, 2};
}

void test_eqjobs_remove_dir_step_matches_reference() {
    for (int variant = 0; variant < 4; variant++) {
        LegacyRig L;
        NewRig N;
        buildTree(L.sd, 2, 3, variant == 1 ? 40 : 5);
        buildTree(N.sd, 2, 3, variant == 1 ? 40 : 5);
        L.sd.removeFails = N.sd.removeFails = (variant == 3);
        int guard = 0;
        bool doneA = false, doneB = false;
        while (!(doneA && doneB) && guard++ < 400) {
            int ba = 7, bb = 7;
            doneA = L.hm.removeDirStep("/history", ba);
            doneB = mhhist::removeDirStep(N.fs, "/history", bb);
            TEST_ASSERT_EQUAL(doneA, doneB);
            TEST_ASSERT_EQUAL(ba, bb);
            assertSameVolume(L.sd, N.sd);
            if (variant == 3 && guard > 60) break;   // suppression impossible : le test s'arrete
        }
        if (variant != 3) TEST_ASSERT_TRUE(doneA && doneB);
        // Chemin absent, fichier, dossier vide.
        int a = 5, b = 5;
        TEST_ASSERT_EQUAL(L.hm.removeDirStep("/nope", a), mhhist::removeDirStep(N.fs, "/nope", b));
        TEST_ASSERT_EQUAL(a, b);
    }
    LegacyRig L;
    NewRig N;
    L.sd.files["/f"] = N.sd.files["/f"] = {1};
    L.sd.dirs.insert("/empty"); N.sd.dirs.insert("/empty");
    int a = 5, b = 5;
    TEST_ASSERT_EQUAL(L.hm.removeDirStep("/f", a), mhhist::removeDirStep(N.fs, "/f", b));
    TEST_ASSERT_EQUAL(a, b);
    TEST_ASSERT_EQUAL(4, b);                       // un fichier supprime = un point de budget
    TEST_ASSERT_EQUAL(L.hm.removeDirStep("/empty", a), mhhist::removeDirStep(N.fs, "/empty", b));
    assertSameVolume(L.sd, N.sd);
}

void test_eqjobs_remove_dir_recursive_matches_reference() {
    LegacyRig L;
    NewRig N;
    buildTree(L.sd, 2, 4, 9);
    buildTree(N.sd, 2, 4, 9);
    L.hm.removeDirRecursive("/history");
    mhhist::removeDirRecursive(N.fs, "/history");
    assertSameVolume(L.sd, N.sd);
    TEST_ASSERT_EQUAL(0, N.sd.files.size());
    TEST_ASSERT_EQUAL(0, N.sd.dirs.size());
}

void test_eqjobs_create_tree_matches_reference() {
    // Volume vide, /history deja la, arborescence complete, parent impossible a creer.
    for (int variant = 0; variant < 4; variant++) {
        LegacyRig L;
        NewRig N;
        if (variant >= 1) { L.sd.dirs.insert("/history"); N.sd.dirs.insert("/history"); }
        if (variant == 2) {
            for (MemVolume* v : {&L.sd, &N.sd}) { v->dirs.insert("/history/indoor"); v->dirs.insert("/history/outdoor"); }
        }
        if (variant == 3) { L.sd.files["/history"] = N.sd.files["/history"] = {1}; }   // /history est un fichier
        L.hm.createSdStructure();
        mhhist::createHistoryTree(N.fs, "/history", "/history/indoor", "/history/outdoor");
        assertSameVolume(L.sd, N.sd);
    }
}

// Passes de ClearJob / service() : meme etat de la carte apres CHAQUE passage.
void test_eqjobs_clear_job_matches_reference() {
    for (int variant = 0; variant < 4; variant++) {
        LegacyRig L;
        NewRig N;
        buildTree(L.sd, 2, 3, variant == 1 ? 30 : 4);
        buildTree(N.sd, 2, 3, variant == 1 ? 30 : 4);
        for (MemVolume* v : {&L.lfs}) { v->files["/history/indoor_recent.dat"] = {1}; }
        if (variant == 2) { L.mgr.available = false; }
        bool sdOk = (variant != 2);
        if (variant == 3) {
            // Dossiers vides impossibles a supprimer : aucun progres, abandon au 21e passage.
            for (MemVolume* v : {&L.sd, &N.sd}) { v->files.clear(); v->removeFails = true; }
        }
        mhhist::ClearJob job;
        TEST_ASSERT_FALSE(job.pending());
        L.hm.requestClear();
        job.request();
        TEST_ASSERT_EQUAL(L.hm.isClearing(), job.pending());
        int ramClears = 0, treeDone = 0;
        int passes = 0;
        while ((L.hm.isClearing() || job.pending()) && passes < 120) {
            L.hm.service();
            job.service(N.fs, "/history", sdOk, [&]() { ramClears++; },
                        [&]() {
                            treeDone++;
                            mhhist::createHistoryTree(N.fs, "/history", "/history/indoor", "/history/outdoor");
                        });
            passes++;
            TEST_ASSERT_EQUAL(L.hm.isClearing(), job.pending());
            // La reference recree aussi l'arborescence a la fin ; on compare apres chaque passage.
            assertSameVolume(L.sd, N.sd);
        }
        if (variant == 3) TEST_ASSERT_EQUAL(21, passes);
        if (variant == 0 || variant == 1) {
            TEST_ASSERT_FALSE(job.pending());
            TEST_ASSERT_EQUAL(1, ramClears);
            TEST_ASSERT_EQUAL(1, treeDone);
        }
        if (variant == 2) {
            TEST_ASSERT_EQUAL(1, ramClears);
            TEST_ASSERT_EQUAL(0, treeDone);
            TEST_ASSERT_EQUAL(1, passes);
        }
    }
}

// Carte qui refuse de supprimer les fichiers : la reference legacy reboucle sans fin (defaut
// corrige en 1.63.1), le nouveau code doit abandonner puis accepter une nouvelle demande.
void test_clear_job_aborts_when_removals_fail() {
    NewRig N;
    buildTree(N.sd, 2, 3, 4);
    N.sd.removeFails = true;
    mhhist::ClearJob job;
    int treeDone = 0;
    auto clearRam = []() {};
    auto onTree = [&]() { treeDone++; };
    job.request();
    int passes = 0;
    while (job.pending() && passes < 120) { job.service(N.fs, "/history", true, clearRam, onTree); passes++; }
    TEST_ASSERT_FALSE(job.pending());       // abandon, pas de boucle infinie
    TEST_ASSERT_EQUAL(21, passes);
    TEST_ASSERT_EQUAL(0, treeDone);
    // Nouvelle demande, carte de nouveau saine : redemarrage propre jusqu'au bout.
    N.sd.removeFails = false;
    job.request();
    passes = 0;
    while (job.pending() && passes < 120) { job.service(N.fs, "/history", true, clearRam, onTree); passes++; }
    TEST_ASSERT_FALSE(job.pending());
    TEST_ASSERT_EQUAL(1, treeDone);
    TEST_ASSERT_FALSE(N.sd.exists("/history/indoor/2026"));
}

// --- Migration des anciens CSV -------------------------------------------------------------------
static std::string csvText(time_t start, int n, bool junk) {
    std::string s = "Timestamp,Temperature,Humidity,Pressure\n";
    for (int i = 0; i < n; i++) {
        char b[80];
        snprintf(b, sizeof(b), "%ld,%.1f,%.1f,%.1f\n", (long)(start + i * 300), 10.f + i * 0.5f, 40.f + (i % 7), 1000.f + (i % 3));
        s += b;
        if (junk && i % 5 == 2) s += "ligne illisible;;\n";
    }
    return s;
}

static void putCsv(LegacyRig& L, NewRig& N, const char* path, const std::string& content) {
    L.sd.files[path] = N.sd.files[path] = std::vector<uint8_t>(content.begin(), content.end());
}

void test_eqjobs_migrate_csv_matches_reference() {
    for (int variant = 0; variant < 5; variant++) {
        LegacyRig L;
        NewRig N;
        for (MemVolume* v : {&L.sd, &N.sd}) v->dirs.insert("/history");
        putCsv(L, N, "/history/2026-11-01.csv", csvText(mkUtc(2026, 11, 1, 8), 30, variant == 1));
        putCsv(L, N, "/history/2026-11-02.csv", csvText(mkUtc(2026, 11, 2, 0), 5, true));
        putCsv(L, N, "/history/2026-11-03.csv", "Timestamp,Temperature,Humidity,Pressure\n");   // en-tete seul
        putCsv(L, N, "/history/2026-11-04.csv", "");                                            // vide
        putCsv(L, N, "/history/readme.txt", "ignore");
        // Journee qui chevauche deux jours locaux : tout part dans le .bin du premier jour.
        putCsv(L, N, "/history/2026-11-06.csv", csvText(mkUtc(2026, 11, 6, 23, 0), 20, false));
        // Journee deja archivee en binaire : le CSV doit rester.
        const time_t d7 = mkUtc(2026, 11, 7, 9);
        L.hm.saveToSdBinary(inRec(d7, 1.f, 2.f, 3.f));
        N.in.append(d7, mhhist::BinRecord{(uint32_t)d7, 1.f, 2.f, 3.f}, 0);
        putCsv(L, N, "/history/2026-11-07.csv", csvText(mkUtc(2026, 11, 7, 0), 4, false));
        if (variant == 2) { L.sd.failOpenForWrite = N.sd.failOpenForWrite = true; }   // carte en lecture seule
        if (variant == 3) { L.sd.quota = N.sd.quota = 300; }                          // carte pleine en route
        if (variant == 4) { L.mgr.mounted = false; }
        L.hm.migrateCsvToBinary();
        mhhist::migrateCsvToBinary(N.in);
        assertSameVolume(L.sd, N.sd);
    }
}

// --- Previsions archivees ------------------------------------------------------------------------
void test_eqjobs_forecast_matches_reference() {
    LegacyRig L;
    NewRig N;
    Rng r(3);
    const char* texts[] = {"Ensoleille", "Pluie \"fine\" et vent", "Neige\nlourde\tpar\rendroits", "Chemin C:\\tmp",
                           "Orage tres tres long : une description volontairement beaucoup trop longue pour tenir "
                           "dans le tampon de 256 octets reserve a la ligne JSON, afin de verifier que la troncature "
                           "reste strictement identique d'un code a l'autre, quelle que soit la longueur du texte "
                           "fourni par la source de previsions et meme au-dela de deux cents caracteres.",
                           ""};
    for (int i = 0; i < 40; i++) {
        legacy::ForecastRecord a;
        ForecastRecord b;
        const uint32_t day = 20261101u + r.below(40);
        a.target_day = b.target_day = (day % 100 > 30) ? 20261201u + r.below(20) : day;
        a.issued_ts = b.issued_ts = 1790000000u + r.below(100000);
        a.temp_min = b.temp_min = -5.f + (float)r.below(300) / 10.f;
        a.temp_max = b.temp_max = a.temp_min + (float)r.below(150) / 10.f;
        a.description = b.description = texts[r.below(6)];
        L.hm.addForecast(a);
        mhhist::addForecast(N.fs, b);
        assertSameVolume(L.sd, N.sd);
    }
    for (int k = 0; k < 25; k++) {
        const time_t from = mkUtc(2026, 10, 25) + (time_t)r.below(30 * 86400);
        const time_t to = from + (time_t)r.below(25 * 86400);
        std::vector<std::string> ea, eb;
        L.hm.forecastHistoryRaw(from, to, [&](const char* s) { ea.push_back(s); });
        mhhist::forecastHistoryRaw(N.fs, from, to, [&](const char* s) { eb.push_back(s); });
        TEST_ASSERT_TRUE(ea == eb);
    }
    // Fichier parasite, espaces autour du JSON, carte retiree, dossier absent.
    for (MemVolume* v : {&L.sd, &N.sd}) {
        v->files["/history/forecast/2026-11-09.json"] = {' ', '\n', '{', '}', ' ', '\n'};
        v->files["/history/forecast/notes.txt"] = {'x'};
        v->files["/history/forecast/2026-11-10.json"] = {};
    }
    std::vector<std::string> ea, eb;
    L.hm.forecastHistoryRaw(mkUtc(2026, 11, 1), mkUtc(2026, 12, 31), [&](const char* s) { ea.push_back(s); });
    mhhist::forecastHistoryRaw(N.fs, mkUtc(2026, 11, 1), mkUtc(2026, 12, 31), [&](const char* s) { eb.push_back(s); });
    TEST_ASSERT_TRUE(ea == eb);
    TEST_ASSERT_TRUE(ea.size() > 5);
    L.mgr.mounted = false;
    N.fs.mounted = false;
    ea.clear(); eb.clear();
    L.hm.forecastHistoryRaw(0, 4000000000, [&](const char* s) { ea.push_back(s); });
    mhhist::forecastHistoryRaw(N.fs, 0, 4000000000, [&](const char* s) { eb.push_back(s); });
    TEST_ASSERT_EQUAL(0, ea.size());
    TEST_ASSERT_EQUAL(0, eb.size());
}

void run_equiv_jobs() {
    RUN_TEST(test_eqjobs_remove_dir_step_matches_reference);
    RUN_TEST(test_eqjobs_remove_dir_recursive_matches_reference);
    RUN_TEST(test_eqjobs_create_tree_matches_reference);
    RUN_TEST(test_eqjobs_clear_job_matches_reference);
    RUN_TEST(test_clear_job_aborts_when_removals_fail);
    RUN_TEST(test_eqjobs_migrate_csv_matches_reference);
    RUN_TEST(test_eqjobs_forecast_matches_reference);
}
