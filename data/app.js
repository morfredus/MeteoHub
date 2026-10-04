// Affichage de l'alerte météo sur le dashboard
let current_alert_payload = null;

function getAlertThemeClass(level) {
    if (level >= 3) return 'alert-level-red';
    if (level === 2) return 'alert-level-orange';
    if (level === 1) return 'alert-level-yellow';
    return 'alert-level-none';
}

function formatAlertValidity(startUnix, endUnix) {
    if (!Number.isFinite(startUnix) || !Number.isFinite(endUnix) || startUnix <= 0 || endUnix <= 0) {
        return 'Validité : non précisée';
    }

    const startText = new Date(startUnix * 1000).toLocaleString('fr-FR');
    const endText = new Date(endUnix * 1000).toLocaleString('fr-FR');
    return `Validité : du ${startText} au ${endText}`;
}

function applyAlertCardTheme(level) {
    const alertCard = document.getElementById('alertCard');
    if (!alertCard) return;

    alertCard.classList.remove('alert-level-none', 'alert-level-yellow', 'alert-level-orange', 'alert-level-red');
    alertCard.classList.add(getAlertThemeClass(level));
}

function renderSensorValidityBadge(isValid) {
    const badge = document.getElementById('sensorInvalidBadge');
    if (!badge) return;

    if (isValid === false) {
        badge.hidden = false;
    } else {
        badge.hidden = true;
    }
}

function updateAlertDetailsButton(enabled) {
    const detailsBtn = document.getElementById('alertDetailsBtn');
    if (!detailsBtn) return;

    detailsBtn.disabled = !enabled;
}

function renderAlertCard(data, isDetailed) {
    const alertText = document.getElementById('alertText');
    const alertValidity = document.getElementById('alertValidity');
    if (!alertText || !alertValidity) return;

    if (data.alert_active || data.active) {
        const level = Number.isFinite(data.alert_severity) ? data.alert_severity : (Number.isFinite(data.severity) ? data.severity : 0);
        const levelLabel = data.alert_level_label_fr || 'Alerte';
        const event = data.alert_event_fr || data.event_fr || data.alert_event || data.event || 'Alerte météo';
        const senderValue = data.alert_sender || data.sender || '';
        const sender = senderValue ? ` • Source: ${senderValue}` : '';
        const detailsText = data.description_fr || data.alert_description_fr || "";
        const details = isDetailed && detailsText ? ` - ${detailsText}` : "";

        alertText.textContent = `${levelLabel} (${level}) - ${event}${sender}${details}`;
        alertText.style.fontWeight = '700';
        alertValidity.textContent = formatAlertValidity(data.alert_start_unix || data.start_unix, data.alert_end_unix || data.end_unix);
        applyAlertCardTheme(level);
        updateAlertDetailsButton(true);
    } else {
        alertText.textContent = 'Aucune alerte météo en cours.';
        alertText.style.fontWeight = '500';
        alertValidity.textContent = 'Validité : --';
        applyAlertCardTheme(0);
        updateAlertDetailsButton(false);
    }
}

function openAlertModal() {
    const modal = document.getElementById('alertModal');
    const body = document.getElementById('alertModalBody');
    if (!modal || !body) return;

    if (!current_alert_payload || !(current_alert_payload.active || current_alert_payload.alert_active)) {
        body.textContent = 'Aucune alerte active.';
    } else {
        const level = Number.isFinite(current_alert_payload.severity) ? current_alert_payload.severity : current_alert_payload.alert_severity;
        const levelLabel = current_alert_payload.alert_level_label_fr || 'Alerte';
        const event = current_alert_payload.event_fr || current_alert_payload.alert_event_fr || current_alert_payload.event || current_alert_payload.alert_event || 'Alerte météo';
        const senderValue = current_alert_payload.sender || current_alert_payload.alert_sender || 'Inconnu';
        const description = current_alert_payload.description_fr || current_alert_payload.alert_description_fr || 'Aucune description détaillée fournie.';
        const validity = formatAlertValidity(current_alert_payload.start_unix || current_alert_payload.alert_start_unix, current_alert_payload.end_unix || current_alert_payload.alert_end_unix);

        body.innerHTML = `<p><strong>${levelLabel} (${level})</strong> - ${event}</p><p><strong>Source :</strong> ${senderValue}</p><p><strong>${validity}</strong></p><p>${description}</p><p><strong>Consigne :</strong> Surveillez l’évolution locale et limitez les déplacements non essentiels.</p>`;
    }

    modal.classList.add('open');
    modal.setAttribute('aria-hidden', 'false');
}

function closeAlertModal() {
    const modal = document.getElementById('alertModal');
    if (!modal) return;

    modal.classList.remove('open');
    modal.setAttribute('aria-hidden', 'true');
}

function initAlertModal() {
    const detailsBtn = document.getElementById('alertDetailsBtn');
    const closeBtn = document.getElementById('alertModalClose');
    const modal = document.getElementById('alertModal');

    if (detailsBtn) detailsBtn.addEventListener('click', openAlertModal);
    if (closeBtn) closeBtn.addEventListener('click', closeAlertModal);
    if (modal) {
        modal.addEventListener('click', (event) => {
            if (event.target === modal) {
                closeAlertModal();
            }
        });
    }

    document.addEventListener('keydown', (event) => {
        if (event.key === 'Escape') {
            closeAlertModal();
        }
    });
}

async function fetchAlert() {
    const alertText = document.getElementById('alertText');
    if (!alertText) return;

    try {
        const res = await fetch('/api/alert');
        const data = await res.json();
        current_alert_payload = data;
        renderAlertCard(data, true);
    } catch (e) {
        alertText.textContent = "Erreur lors de la récupération de l'alerte météo.";
        updateAlertDetailsButton(false);
        applyAlertCardTheme(0);
    }
}
const LIVE_REFRESH_MS = 5000;
const ALERT_REFRESH_MS = 15 * 60 * 1000;
const STATS_REFRESH_MS = 15000;

// Auto-rafraîchissement de la page Historique : calé sur la cadence D'ENREGISTREMENT
// (une mesure toutes les 5 min), pas sur un intervalle court. La cadence réelle est
// lue depuis /api/history (champ measurement_interval_s, source unique côté firmware) ;
// on ajoute une petite marge pour tomber juste après l'écriture d'un nouveau point.
// Valeur par défaut 5 min avant la première réponse.
let measurementIntervalMs = 300000;
const LONGTERM_REFRESH_MARGIN_MS = 20000;

function getPageName() {
    return document.body?.dataset?.page || 'dashboard';
}

function isStatsPage() {
    return getPageName() === 'stats';
}

function setText(id, txt) {
    const el = document.getElementById(id);
    if (el) el.textContent = txt;
}

// Instant du dernier relevé extérieur, au format « jj/mm - hh:mm:ss ». Le hub
// ESP32 n'a pas forcément d'horloge fiable, mais il fournit l'âge de la trame
// (age_ms) ; on reconstruit donc l'heure côté navigateur : maintenant - âge.
function formatMeasureTime(ageMs) {
    const d = new Date(Date.now() - (ageMs || 0));
    const p = (n) => String(n).padStart(2, '0');
    return p(d.getDate()) + '/' + p(d.getMonth() + 1)
        + ' - ' + p(d.getHours()) + ':' + p(d.getMinutes()) + ':' + p(d.getSeconds());
}

// Met à jour l'affichage station météo à partir des blocs in / out / effective
// de /api/live. La provenance n'est jamais perdue : une valeur de secours issue
// de l'intérieur est signalée comme telle, jamais présentée comme extérieure.
function updateStation(data) {
    const eff = data.effective || {};
    const inb = data.in || {};
    const outb = data.out || {};
    const et = eff.temp || {};
    const eh = eff.hum || {};

    // Héros : température/humidité extérieures effectives.
    setText('heroTemp', et.valid ? Number(et.value).toFixed(1) : '--');
    setText('heroHum', eh.valid ? Number(eh.value).toFixed(0) : '--');

    // Au boot, aucune trame OUT n'a encore été reçue : la valeur "out" éventuelle
    // vient d'un seed disque (ancienne), pas d'une mesure réelle. On la traite donc
    // comme "en attente", sans afficher ni sa valeur ni un horodatage fantaisiste.
    const awaiting = !!outb.awaiting_first;
    const awaitMins = Math.max(1, Math.round((outb.interval_s || 300) / 60));

    const badge = document.getElementById('heroBadge');
    const state = document.getElementById('heroState');
    if (badge && state) {
        const st = et.state;
        if (awaiting) {
            badge.hidden = false;
            badge.textContent = 'secours intérieur';
            badge.className = 'hero-badge fallback';
            state.textContent = 'En attente du premier relevé extérieur après redémarrage (max '
                + awaitMins + ' min)';
            state.className = 'hero-state warn';
        } else if (!et.valid) {
            badge.hidden = true;
            state.textContent = 'Aucune donnée disponible';
            state.className = 'hero-state warn';
        } else if (st === 'fallback') {
            badge.hidden = false;
            badge.textContent = 'secours intérieur';
            badge.className = 'hero-badge fallback';
            state.textContent = 'Extérieur indisponible, valeur intérieure affichée';
            state.className = 'hero-state warn';
        } else if (st === 'stale') {
            badge.hidden = false;
            badge.textContent = 'donnée ancienne';
            badge.className = 'hero-badge stale';
            state.textContent = 'Donnée extérieure ancienne';
            state.className = 'hero-state stale';
        } else {
            badge.hidden = true;
            state.textContent = 'Donnée extérieure à jour';
            state.className = 'hero-state ok';
        }
    }

    // Bloc OUT (météo extérieure réelle). En attente de la 1re trame, on n'affiche
    // PAS la valeur seedée depuis le disque : elle induirait en erreur.
    const outShow = outb.valid && !awaiting;
    setText('outTemp', outShow ? Number(outb.temp).toFixed(1) : '--');
    setText('outHum', outShow ? Number(outb.hum).toFixed(0) : '--');
    setText('outPres', (outShow && outb.pres > 300) ? Number(outb.pres).toFixed(0) : '--');
    const fresh = document.getElementById('outFreshness');
    if (fresh) {
        if (awaiting) {
            // Boot : pas encore de trame reçue. Pas une panne : attente de la 1re
            // transmission (bornée par la cadence). Aucun horodatage affiché.
            fresh.textContent = 'en attente du premier relevé après redémarrage (max '
                + awaitMins + ' min)';
            fresh.className = 'freshness';
        } else if (!outb.valid) {
            fresh.textContent = 'sonde extérieure absente';
            fresh.className = 'freshness warn';
        } else if (outb.fresh) {
            fresh.textContent = 'à jour (' + formatMeasureTime(outb.age_ms) + ')';
            fresh.className = 'freshness ok';
        } else {
            const mins = Math.round((outb.age_ms || 0) / 60000);
            fresh.textContent = 'dernière trame il y a ' + mins
                + ' min (' + formatMeasureTime(outb.age_ms) + ')';
            fresh.className = 'freshness stale';
        }
    }

    // Version du firmware de la sonde (trame v4) : masquée tant qu'elle est inconnue.
    const fwRow = document.getElementById('outFirmwareRow');
    if (fwRow) {
        fwRow.hidden = !outb.firmware;
        if (outb.firmware) setText('outFirmware', 'v' + outb.firmware);
    }

    // Batterie de la sonde déportée + alerte pile faible.
    const batteryRow = document.getElementById('outBatteryRow');
    const batteryAlert = document.getElementById('outBatteryAlert');
    if (batteryRow && batteryAlert) {
        if (outb.has_battery) {
            batteryRow.hidden = false;
            setText('outBattery', Number(outb.battery_pct).toFixed(0));
            // Tension de la cellule (V) en plus du pourcentage : c'est la valeur
            // que Fred lit au multimètre pour caler la sonde.
            setText('outBatteryV', (typeof outb.battery_v === 'number')
                ? Number(outb.battery_v).toFixed(2) : '--');
            if (outb.battery_low) {
                batteryAlert.hidden = false;
                batteryAlert.textContent = `Pile de la sonde faible (${Number(outb.battery_pct).toFixed(0)} %) : à remplacer`;
            } else {
                batteryAlert.hidden = true;
            }
        } else {
            batteryRow.hidden = true;
            batteryAlert.hidden = true;
        }
    }

    // Bloc IN (confort intérieur).
    setText('inTemp', inb.valid ? Number(inb.temp).toFixed(1) : '--');
    setText('inHum', inb.valid ? Number(inb.hum).toFixed(0) : '--');
}

async function fetchLive() {
    try {
        const res = await fetch('/api/live');
        const data = await res.json();

        updateStation(data);
        renderSensorValidityBadge(data.sensor_valid);

        const status = document.getElementById('status');
        if (status) {
            status.textContent = 'En ligne';
            status.style.color = '#0f0';
        }

    } catch (e) {
        const status = document.getElementById('status');
        if (status) {
            status.textContent = 'Déconnecté';
            status.style.color = '#f00';
        }
    }
}

async function fetchSystem() {
    try {
        const res = await fetch('/api/system');
        const data = await res.json();
        const version = document.getElementById('version');
        if (version) version.textContent = data.project_version || data.version || '--';
    } catch (e) {}
}

// Remplit un tableau de résumé (min/moy/max) pour un contexte donné (IN ou OUT).
// count == 0 => aucune mesure disponible pour ce contexte sur la période.
function fillStatsTable(tbodyId, data) {
    const tbody = document.getElementById(tbodyId);
    if (!tbody) return;
    if (!data || !data.count || data.count <= 0 || !data.temp) {
        tbody.innerHTML = '<tr><td colspan="4">Aucune mesure disponible</td></tr>';
        return;
    }
    // Nombre de mesures derrière la synthèse : rend visible la couverture réelle
    // de chaque source (l'extérieur peut en avoir beaucoup moins que l'intérieur).
    tbody.innerHTML = `
        <tr><td colspan="4" class="stats-count">${data.count} mesure(s)</td></tr>
        <tr>
            <td>Température (°C)</td>
            <td>${data.temp.min.toFixed(1)}</td>
            <td>${data.temp.avg.toFixed(1)}</td>
            <td>${data.temp.max.toFixed(1)}</td>
        </tr>
        <tr>
            <td>Humidité (%)</td>
            <td>${data.hum.min.toFixed(0)}</td>
            <td>${data.hum.avg.toFixed(0)}</td>
            <td>${data.hum.max.toFixed(0)}</td>
        </tr>
        <tr>
            <td>Pression (hPa)</td>
            <td>${data.pres.min.toFixed(0)}</td>
            <td>${data.pres.avg.toFixed(0)}</td>
            <td>${data.pres.max.toFixed(0)}</td>
        </tr>
    `;
}

async function fetchStats() {
    if (!isStatsPage()) return;

    try {
        // IN et OUT côte à côte : deux requêtes (défaut = IN, ctx=out = extérieur).
        const [inData, outData] = await Promise.all([
            fetch('/api/stats').then((r) => r.json()),
            fetch('/api/stats?ctx=out').then((r) => r.json())
        ]);
        fillStatsTable('statsBodyIn', inData);
        fillStatsTable('statsBodyOut', outData);

        // Tendance MÉTÉO = extérieur (OUT) : c'est la source de vérité météo.
        const data = outData;

        const trendBody = document.getElementById('trendBody');
        const trendGlobal = document.getElementById('trendGlobal');
        if (trendBody) {
            const trend = data.trend || {};
            const t = trend.temp || {};
            const h = trend.hum || {};
            const p = trend.pres || {};
            const available48h = !!trend.available_48h;
            const globalLabel = trend.global_label_fr || 'Tendance stable';

            // Affiche la variation et la direction ensemble (ex: "+0.8 °C ↗")
            const arrow = (direction) => {
                if (direction === 'hausse') return '↗';
                if (direction === 'baisse') return '↘';
                if (direction === 'indisponible') return '';
                return '→';
            };
            const cell = (delta, direction, unit, decimals) => {
                if (direction === 'indisponible') return 'N/D';
                const sign = delta > 0 ? '+' : '';
                return `${sign}${(delta ?? 0).toFixed(decimals)} ${unit} ${arrow(direction)}`;
            };

            trendBody.innerHTML = `
                <tr>
                    <td>Température</td>
                    <td>${cell(t.delta_1h, t.direction_1h, '°C', 1)}</td>
                    <td>${cell(t.delta_12h, t.direction_12h, '°C', 1)}</td>
                    <td>${cell(t.delta_24h, t.direction_24h, '°C', 1)}</td>
                    <td>${available48h ? cell(t.delta_48h, t.direction_48h, '°C', 1) : 'N/D'}</td>
                </tr>
                <tr>
                    <td>Humidité</td>
                    <td>${cell(h.delta_1h, h.direction_1h, '%', 1)}</td>
                    <td>${cell(h.delta_12h, h.direction_12h, '%', 1)}</td>
                    <td>${cell(h.delta_24h, h.direction_24h, '%', 1)}</td>
                    <td>${available48h ? cell(h.delta_48h, h.direction_48h, '%', 1) : 'N/D'}</td>
                </tr>
                <tr>
                    <td>Pression</td>
                    <td>${cell(p.delta_1h, p.direction_1h, 'hPa', 1)}</td>
                    <td>${cell(p.delta_12h, p.direction_12h, 'hPa', 1)}</td>
                    <td>${cell(p.delta_24h, p.direction_24h, 'hPa', 1)}</td>
                    <td>${available48h ? cell(p.delta_48h, p.direction_48h, 'hPa', 1) : 'N/D'}</td>
                </tr>
            `;

            if (trendGlobal) {
                trendGlobal.innerHTML = `<strong>Tendance générale (1h/12h/24h${available48h ? '/48h' : ''}) :</strong> ${globalLabel}`;
            }
        }

        const status = document.getElementById('status');
        if (status) {
            status.textContent = 'En ligne';
            status.style.color = '#0f0';
        }

        const updated = document.getElementById('statsUpdated');
        if (updated) updated.textContent = 'Mis à jour à ' + new Date().toLocaleTimeString('fr-FR');
    } catch (e) {
        console.error('Erreur stats', e);
    }
}

// Rafraîchissement automatique de la page Statistiques, activable/désactivable.
let statsRefreshTimer = null;

function setStatsAutoRefresh(enabled) {
    if (statsRefreshTimer) { clearInterval(statsRefreshTimer); statsRefreshTimer = null; }
    if (enabled) statsRefreshTimer = setInterval(fetchStats, STATS_REFRESH_MS);
}

function initStatsControls() {
    const toggle = document.getElementById('statsAutoRefresh');
    const refreshBtn = document.getElementById('statsRefreshNow');

    // Actif par défaut (case cochée dans le HTML).
    setStatsAutoRefresh(!toggle || toggle.checked);
    if (toggle) toggle.addEventListener('change', () => setStatsAutoRefresh(toggle.checked));
    // Le bouton « Actualiser » reste utile quand la mise à jour auto est désactivée.
    if (refreshBtn) refreshBtn.addEventListener('click', fetchStats);
}

// Seuils de bruit négligeable par grandeur : en dessous, un écart ponctuel n'est
// jamais considéré comme aberrant (évite de « nettoyer » les micro-variations).
const OUTLIER_FLOOR = { temp: 1.0, hum: 6, pres: 1.0 };

// Détection de valeurs aberrantes fondée sur la cohérence temporelle (et non sur
// un seuil fixe) : un point est écarté (mis à null) s'il s'éloigne fortement de
// SES DEUX voisins alors que ceux-ci restent cohérents entre eux, c.-à-d. un pic
// ou un creux d'un seul point suivi d'un retour immédiat à la normale. Les points
// valides sont alors reliés directement (spanGaps). Les données brutes ne sont pas
// modifiées : seule leur exploitation (tracé et statistiques) est adaptée.
function filterOutliers(values, floor) {
    const n = values.length;
    if (n < 3) return values.slice();
    const out = values.slice();
    for (let i = 1; i < n - 1; i++) {
        const prev = values[i - 1], cur = values[i], next = values[i + 1];
        if (prev === null || cur === null || next === null) continue;
        if (prev === undefined || cur === undefined || next === undefined) continue;
        const dPrev = Math.abs(cur - prev);
        const dNext = Math.abs(cur - next);
        const jump = Math.min(dPrev, dNext);          // amplitude aller-retour du pic
        const neighborGap = Math.abs(prev - next);    // cohérence des voisins entre eux
        if (jump > floor && jump > 3 * neighborGap) {
            out[i] = null; // anomalie manifeste : exclue du tracé et des stats
        }
    }
    return out;
}

// ------------------------------------------------------------------
// Page Historique : graphe SVG, même représentation que morfAnalytics
// (Météo > Graphiques) : mêmes filtres, mêmes couleurs, mêmes échelles.
//   - Grandeurs : sélection libre par cases à cocher. Une seule -> vue mono
//     (IN/OUT en deux couleurs). Plusieurs -> superposées, chacune son axe et
//     sa couleur (la première à gauche, les autres à droite), l'intérieur en
//     trait plus fin et atténué.
//   - Échelles toujours dynamiques (min/max de la période + 8 % de marge).
//   - Période : glissante (6 h ... 30 j) ou libre (jour + heure, pas de 5 min).
//   - Points écartés (pic isolé, cf. filterOutliers) : croix grises avec leur
//     motif au survol, jamais comptés dans les courbes ni les échelles.
// Différence assumée avec morfAnalytics : pas de bloc « Événements détectés »
// (croisements, régimes), calculés côté Pi (MeteoEvents), hors de portée de l'ESP32.
// ------------------------------------------------------------------
const LONGTERM_TARGET_POINTS = 250; // points visés/requête (marge sous la limite ESP32)
let longtermRefreshTimer = null;

// Calcule un intervalle d'agrégation (secondes) pour tenir ~LONGTERM_TARGET_POINTS.
function computeInterval(durationSeconds) {
    let interval = Math.floor(durationSeconds / LONGTERM_TARGET_POINTS);
    if (interval < 60) interval = 60; // pas de tranche plus fine qu'une minute
    // Une tranche plus courte que la cadence d'enregistrement tombe régulièrement
    // sans aucune mesure : trou périodique dans la courbe. Avec 1,5 x la cadence,
    // chaque tranche contient au moins une mesure, même avec un peu de gigue.
    const minInterval = Math.ceil(1.5 * measurementIntervalMs / 1000);
    if (interval < minInterval) interval = minInterval;
    return interval;
}

function showChartLoading(visible) {
    const el = document.getElementById('chartLoading');
    if (el) el.hidden = !visible;
}

// Séries filtrées des sources A (OUT, ou la seule source) et B (IN en vue IN+OUT)
// conservées pour la synthèse (activation de la bascule sans nouveau chargement).
let longtermFilteredA = null;
let longtermFilteredB = null;

// Durée maximale d'une période encore rafraîchie automatiquement (48 h). Au-delà,
// chaque rafraîchissement relancerait un scan de plusieurs fichiers CSV sur la carte
// SD : on évite de le répéter en continu (l'utilisateur peut recharger manuellement).
const LONGTERM_AUTOREFRESH_MAX_SECONDS = 172800;

function initHistoryPage() {
    const LS = 'meteohub.graphs.';
    // [clé, libellé, unité, décimales, couleur] : couleurs de morfAnalytics (thème sombre).
    const METRICS = [
        ['temp', 'Température', '°C', 1, '#e6a54e'],
        ['hum', 'Humidité', '%', 0, '#7ee0b8'],
        ['pres', 'Pression', 'hPa', 1, '#c58bf2']
    ];
    const METRIC_KEYS = METRICS.map((m) => m[0]);
    const SOURCES = [['out', 'Extérieur'], ['in', 'Intérieur'], ['both', 'Intérieur + Extérieur']];
    const PERIODS = [['6 h', 6], ['12 h', 12], ['24 h', 24], ['3 j', 72], ['7 j', 168], ['30 j', 720]];
    // Couleur = grandeur, quel que soit le filtre ; IN/OUT : épaisseur et opacité.
    const COL_AXIS = '#99a1ad', COL_SUSP = '#8a929e', COL_TIP = '#0e1013', COL_INK = '#e7e9ec';
    // Période libre : bornes alignées sur 5 min (cadence de la sonde), au plus un an.
    const STEP_S = 300;
    const MAX_SPAN_S = 366 * 86400;
    // Cadence ~5 min : plancher de connexion des points (trait coupé sur un vrai silence).
    const CONNECT_MIN_S = 20 * 60;
    const QUAL_LABEL = { pic: 'pic isolé' };

    const $ = (s) => document.querySelector(s);
    const lsGet = (k) => { try { return localStorage.getItem(LS + k); } catch (e) { return null; } };
    const lsSet = (k, v) => { try { localStorage.setItem(LS + k, v); } catch (e) { /* stockage indisponible */ } };
    const lsDel = (k) => { try { localStorage.removeItem(LS + k); } catch (e) { /* idem */ } };

    function loadMetrics() {
        try {
            const a = JSON.parse(lsGet('metrics') || 'null');
            if (Array.isArray(a)) {
                const f = METRIC_KEYS.filter((k) => a.indexOf(k) >= 0);
                if (f.length) return f;
            }
        } catch (e) { /* valeur corrompue : défaut */ }
        return ['temp'];
    }
    function loadRange() {
        try {
            const r = JSON.parse(lsGet('range') || 'null');
            if (r && r.from > 0 && r.to - r.from >= STEP_S) return { from: +r.from, to: +r.to };
        } catch (e) { /* idem */ }
        return null;
    }
    const S = {
        metrics: loadMetrics(),
        source: lsGet('source') || 'out',
        hours: +(lsGet('hours') || 24),
        range: loadRange(), // null = période glissante ; sinon {from,to} en secondes epoch
        showSuspects: lsGet('susp') !== '0'
    };
    if (!SOURCES.some((s) => s[0] === S.source)) S.source = 'out';

    const metricDef = (k) => METRICS.find((m) => m[0] === k) || METRICS[0];
    const srcLabel = (k) => (SOURCES.find((s) => s[0] === k) || ['', ''])[1];
    const ctxsOf = () => (S.source === 'both' ? ['out', 'in'] : [S.source]);
    function fmtClock(ts) {
        const d = new Date(ts * 1000);
        const sameDay = (Date.now() - d) < 86400000;
        return d.toLocaleString('fr-FR', sameDay
            ? { hour: '2-digit', minute: '2-digit' }
            : { day: '2-digit', month: '2-digit', hour: '2-digit', minute: '2-digit' });
    }
    const fmtFull = (ts) => new Date(ts * 1000).toLocaleString('fr-FR',
        { day: '2-digit', month: '2-digit', hour: '2-digit', minute: '2-digit' });
    function minMax(vals) {
        let mn = Infinity, mx = -Infinity;
        for (const v of vals) { if (v !== null && v !== undefined) { if (v < mn) mn = v; if (v > mx) mx = v; } }
        return [mn, mx];
    }
    const nf = (v, d) => ((v === null || !isFinite(v)) ? '-' : v.toFixed(d));

    // Échelle d'une grandeur : amplitude des données + 8 % (au moins 0,5 ou 1 unité).
    function scaleOf(allv, dec) {
        let [mn, mx] = minMax(allv);
        if (!isFinite(mn)) { mn = 0; mx = 1; }
        const minPad = dec ? 0.5 : 1;
        const pad = Math.max((mx - mn) * 0.08, minPad);
        return { mn, mx, smin: mn - pad, smax: mx + pad };
    }
    const valuesOf = (D, key, ctxs) => {
        let all = [];
        ctxs.forEach((c) => { all = all.concat((D[key][c].v || []).filter((x) => x !== null && x !== undefined)); });
        return all;
    };

    // Géométrie + séries du graphe courant, pour le survol.
    let G = null;

    // Graphe SVG. `series` = [{ts,vals,color,width,opacity,bucket,smin,smax,label,unit,dec,suspects}].
    // `axes` = [{min,max,dec,side:'L'|'R',col}] : échelles affichées à gauche/droite.
    function buildChart(series, axes) {
        const W = 760, H = 240, pT = 12, pB = 24;
        const lefts = axes.filter((a) => a.side === 'L'), rights = axes.filter((a) => a.side === 'R');
        const pL = lefts.length ? 48 : 16;
        const pR = 14 + rights.length * 46;
        let t0 = Infinity, t1 = -Infinity, any = false;
        series.forEach((s) => {
            for (let i = 0; i < s.vals.length; i++) {
                const v = s.vals[i];
                if (v !== null && v !== undefined) { any = true; const t = s.ts[i]; if (t < t0) t0 = t; if (t > t1) t1 = t; }
            }
        });
        if (!any) { G = null; return '<div class="muted">Pas encore de mesure sur cette période.</div>'; }
        if (!(t1 > t0)) t1 = t0 + 1;
        const X = (t) => pL + (W - pL - pR) * ((t - t0) / Math.max(1, (t1 - t0)));
        const Ys = (s, v) => pT + (H - pT - pB) * (1 - (v - s.smin) / Math.max(1e-9, s.smax - s.smin));

        let grid = '', labels = '';
        [0, 0.5, 1].forEach((f) => {
            const y = pT + (H - pT - pB) * (1 - f);
            grid += '<line class="grid-l" x1="' + pL + '" y1="' + y.toFixed(1) + '" x2="' + (W - pR) + '" y2="' + y.toFixed(1) + '"/>';
            lefts.forEach((a) => {
                const val = a.min + (a.max - a.min) * f;
                labels += '<text class="ax" x="' + (pL - 6) + '" y="' + (y + 3).toFixed(1) + '" text-anchor="end" fill="' + a.col + '">' + val.toFixed(a.dec || 0) + '</text>';
            });
            rights.forEach((a, ri) => {
                const val = a.min + (a.max - a.min) * f;
                const xr = (W - pR) + 10 + ri * 46;
                labels += '<text class="ax" x="' + xr + '" y="' + (y + 3).toFixed(1) + '" text-anchor="start" fill="' + a.col + '">' + val.toFixed(a.dec || 0) + '</text>';
            });
        });
        const xt0 = '<text class="ax" x="' + pL + '" y="' + (H - 6) + '">' + fmtClock(t0) + '</text>';
        const xt1 = '<text class="ax" x="' + (W - pR) + '" y="' + (H - 6) + '" text-anchor="end">' + fmtClock(t1) + '</text>';

        let paths = '';
        series.forEach((s) => {
            const gapMax = Math.max((s.bucket > 0 ? s.bucket : 600) * 2.5, CONNECT_MIN_S);
            let d = '', prevT = null;
            for (let i = 0; i < s.ts.length; i++) {
                const v = s.vals[i];
                if (v === null || v === undefined) continue;
                const t = s.ts[i];
                const move = (prevT === null) || ((t - prevT) > gapMax);
                d += (move ? 'M' : 'L') + X(t).toFixed(1) + ' ' + Ys(s, v).toFixed(1) + ' ';
                prevT = t;
            }
            const op = (s.opacity !== undefined ? ' stroke-opacity="' + s.opacity + '"' : '');
            paths += '<path d="' + d + '" fill="none" stroke="' + s.color + '" stroke-width="' + (s.width || 2) + '"' + op + '/>';
            let last = null;
            for (let i = s.ts.length - 1; i >= 0; i--) {
                if (s.vals[i] !== null && s.vals[i] !== undefined) { last = [s.ts[i], s.vals[i]]; break; }
            }
            if (last) paths += '<circle cx="' + X(last[0]).toFixed(1) + '" cy="' + Ys(s, last[1]).toFixed(1) + '" r="3" fill="' + s.color + '"' + op + '/>';
        });

        // Points écartés : croix grises à leur valeur d'origine (ramenée dans le cadre
        // si elle sort de l'échelle, qui ne les prend pas en compte), motif au survol.
        let susp = '';
        if (S.showSuspects) {
            series.forEach((s) => {
                (s.suspects || []).forEach((p) => {
                    const t = p[0], v = p[1], why = QUAL_LABEL[p[2]] || p[2];
                    if (t < t0 || t > t1) return;
                    const x = X(t);
                    const y = Math.max(pT + 3, Math.min(H - pB - 3, Ys(s, v)));
                    susp += '<g stroke="' + COL_SUSP + '" stroke-width="1.6"><title>' + fmtFull(t) + ' · ' + s.label + ' ' +
                        v.toFixed(s.dec) + ' ' + s.unit + ' écarté : ' + why + '</title>' +
                        '<line x1="' + (x - 3.5).toFixed(1) + '" y1="' + (y - 3.5).toFixed(1) + '" x2="' + (x + 3.5).toFixed(1) + '" y2="' + (y + 3.5).toFixed(1) + '"/>' +
                        '<line x1="' + (x - 3.5).toFixed(1) + '" y1="' + (y + 3.5).toFixed(1) + '" x2="' + (x + 3.5).toFixed(1) + '" y2="' + (y - 3.5).toFixed(1) + '"/>' +
                        '<rect x="' + (x - 5).toFixed(1) + '" y="' + (y - 5).toFixed(1) + '" width="10" height="10" fill="transparent" stroke="none"/></g>';
                });
            });
        }

        G = { W, H, pL, pR, pT, pB, t0, t1, series };
        return '<svg id="gsvg" viewBox="0 0 ' + W + ' ' + H + '" preserveAspectRatio="none" role="img">' +
            grid + labels + paths + susp + '<g id="hoverg"></g>' + xt0 + xt1 + '</svg>';
    }

    // Bilan qualité de la période : points écartés par motif (toutes courbes).
    function qualityLine(series) {
        const n = {};
        let total = 0;
        series.forEach((s) => (s.suspects || []).forEach((p) => { total++; n[p[2]] = (n[p[2]] || 0) + 1; }));
        if (!total) return '<p class="qual">Qualité : aucun point écarté sur la période.</p>';
        const parts = Object.keys(n).map((k) => n[k] + ' ' + (QUAL_LABEL[k] || k));
        return '<p class="qual">Qualité : <b>' + total + ' point' + (total > 1 ? 's' : '') + ' écarté' + (total > 1 ? 's' : '') +
            '</b> des courbes (' + parts.join(', ') + '). Données brutes conservées' +
            (S.showSuspects ? ' ; croix grises sur le graphique.' : '.') + '</p>';
    }

    function noteFor(multi) {
        if (multi) {
            return '<p class="note">' + (S.source === 'both'
                ? 'Plusieurs grandeurs sur un axe de temps commun ; chacune a sa propre échelle (la première sélectionnée à gauche, les autres à droite). Intérieur en trait plus fin et atténué. Survolez pour lire les valeurs.'
                : 'Plusieurs grandeurs sur un axe de temps commun, chacune à son échelle. Survolez pour lire les valeurs.') + '</p>';
        }
        return '<p class="note">' + (S.source === 'both'
            ? 'Intérieur en trait plus fin et atténué ; échelle à gauche et à droite.'
            : 'Échelle à gauche et à droite. Points reliés tant que l\'écart reste proche de la cadence ; coupé seulement sur un vrai silence du capteur.') + '</p>';
    }

    // Vue MONO-GRANDEUR : IN/OUT en deux couleurs, échelle numérique à gauche ET à droite.
    function renderSingle(D, key) {
        const md = metricDef(key);
        const ctxs = ctxsOf();
        const sc = scaleOf(valuesOf(D, key, ctxs), md[3]);
        const series = ctxs.map((c) => ({
            ts: D[key][c].ts || [], vals: D[key][c].v || [], color: md[4], width: c === 'in' ? 1.4 : 2.2, opacity: c === 'in' ? 0.55 : 1,
            bucket: D[key][c].bucket_s || 0, smin: sc.smin, smax: sc.smax, label: srcLabel(c),
            unit: md[2], dec: md[3], suspects: D[key][c].suspects || []
        }));
        const axes = [{ min: sc.smin, max: sc.smax, dec: md[3], side: 'L', col: COL_AXIS },
                      { min: sc.smin, max: sc.smax, dec: md[3], side: 'R', col: COL_AXIS }];
        const legend = ctxs.map((c) => '<span class="k"><span class="sw" style="border-color:' + md[4] + ';opacity:' + (c === 'in' ? 0.55 : 1) + ';border-top-width:' + (c === 'in' ? 2 : 3) + 'px"></span>' + srcLabel(c) + '</span>').join('');
        return '<div class="chart"><h3>' + md[1] + ' (' + md[2] + ')</h3><div class="plot">' + buildChart(series, axes) +
            '<div class="tip" hidden></div></div><div class="legend">' + legend + '</div>' + qualityLine(series) + noteFor(false) + '</div>';
    }

    // Vue MULTI-GRANDEURS : un seul graphe, les grandeurs sélectionnées superposées.
    // Chaque grandeur a son axe : la première sélectionnée à gauche, les suivantes à droite.
    function renderAll(D, metrics) {
        const ctxs = ctxsOf();
        const series = [], axes = [], legend = [];
        let drawn = 0;
        metrics.forEach((key) => {
            const m = metricDef(key), col = m[4];
            const allv = valuesOf(D, key, ctxs);
            if (!allv.length) return;
            const sc = scaleOf(allv, m[3]);
            axes.push({ min: sc.smin, max: sc.smax, dec: m[3], side: drawn === 0 ? 'L' : 'R', col });
            drawn++;
            ctxs.forEach((c) => {
                const inner = (c === 'in');
                series.push({
                    ts: D[key][c].ts || [], vals: D[key][c].v || [], color: col,
                    width: inner ? 1.4 : 2.2, opacity: inner ? 0.55 : 1, bucket: D[key][c].bucket_s || 0,
                    smin: sc.smin, smax: sc.smax,
                    label: m[1] + (S.source === 'both' ? (inner ? ' (int)' : ' (ext)') : ''),
                    unit: m[2], dec: m[3], suspects: D[key][c].suspects || []
                });
            });
            const range = nf(sc.mn, m[3]) + '–' + nf(sc.mx, m[3]) + ' ' + m[2];
            ctxs.forEach((c) => {
                const inner = (c === 'in');
                legend.push('<span class="k"><span class="sw" style="border-color:' + col + ';opacity:' + (inner ? 0.55 : 1) + ';border-top-width:' + (inner ? 2 : 3) + 'px"></span>' +
                    m[1] + (S.source === 'both' ? ' ' + (inner ? '(int)' : '(ext)') : '') + (c === ctxs[ctxs.length - 1] ? ' · ' + range : '') + '</span>');
            });
        });
        const names = metrics.map((k) => metricDef(k)[1]).join(' + ');
        const title = names + (S.source === 'both' ? ' (Intérieur + Extérieur)' : ' (' + srcLabel(S.source) + ')');
        const body = series.length ? buildChart(series, axes) : '<div class="muted">Pas encore de mesure sur cette période.</div>';
        return '<div class="chart"><h3>' + title + '</h3><div class="plot">' + body + '<div class="tip" hidden></div></div>' +
            '<div class="legend">' + legend.join('') + '</div>' + qualityLine(series) + noteFor(true) + '</div>';
    }

    // Survol : ligne-guide + infobulle des valeurs à l'instant pointé.
    function attachHover() {
        const svg = $('#gsvg'), tip = document.querySelector('.plot .tip'), hg = $('#hoverg');
        if (!svg || !tip || !hg || !G) return;
        const plot = svg.closest('.plot');
        const leave = () => { tip.hidden = true; hg.innerHTML = ''; };
        svg.addEventListener('mousemove', (ev) => {
            const r = svg.getBoundingClientRect();
            const sx = (ev.clientX - r.left) / r.width * G.W;
            if (sx < G.pL || sx > G.W - G.pR) { leave(); return; }
            const t = G.t0 + (sx - G.pL) / Math.max(1, (G.W - G.pL - G.pR)) * (G.t1 - G.t0);
            const X = (tt) => G.pL + (G.W - G.pL - G.pR) * ((tt - G.t0) / Math.max(1, (G.t1 - G.t0)));
            const Ys = (s, v) => G.pT + (G.H - G.pT - G.pB) * (1 - (v - s.smin) / Math.max(1e-9, s.smax - s.smin));
            let dots = '', rows = '';
            G.series.forEach((s) => {
                let best = -1, bd = Infinity;
                for (let i = 0; i < s.ts.length; i++) {
                    const v = s.vals[i];
                    if (v === null || v === undefined) continue;
                    const d = Math.abs(s.ts[i] - t);
                    if (d < bd) { bd = d; best = i; }
                }
                if (best < 0) return;
                const gapMax = Math.max((s.bucket > 0 ? s.bucket : 600) * 2.5, CONNECT_MIN_S);
                if (bd > gapMax) return; // point trop loin (vrai trou) : on ne l'invente pas
                dots += '<circle cx="' + X(s.ts[best]).toFixed(1) + '" cy="' + Ys(s, s.vals[best]).toFixed(1) + '" r="3.6" fill="' + s.color + '" stroke="' + COL_TIP + '" stroke-width="1.2"/>';
                rows += '<div class="tr"><span class="sw" style="background:' + s.color + (s.opacity !== undefined ? ';opacity:' + s.opacity : '') + '"></span>' +
                    s.label + ' : <b>' + s.vals[best].toFixed(s.dec) + ' ' + s.unit + '</b></div>';
            });
            hg.innerHTML = '<line x1="' + sx.toFixed(1) + '" y1="' + G.pT + '" x2="' + sx.toFixed(1) + '" y2="' + (G.H - G.pB) +
                '" stroke="' + COL_INK + '" stroke-opacity="0.22" stroke-width="1"/>' + dots;
            if (!rows) { tip.hidden = true; return; }
            tip.innerHTML = '<div class="th">' + fmtFull(t) + '</div>' + rows;
            tip.hidden = false;
            const pr = plot.getBoundingClientRect();
            let left = ev.clientX - pr.left + 14, top = ev.clientY - pr.top + 14;
            if (left + tip.offsetWidth > pr.width) left = ev.clientX - pr.left - tip.offsetWidth - 14;
            if (top + tip.offsetHeight > pr.height) top = pr.height - tip.offsetHeight - 4;
            if (top < 0) top = 4;
            tip.style.left = left + 'px'; tip.style.top = top + 'px';
        });
        svg.addEventListener('mouseleave', leave);
    }

    // --- Données --------------------------------------------------------------
    // Fenêtre courante : période libre, sinon fenêtre glissante qui finit maintenant.
    function currentRange() {
        if (S.range) return S.range;
        const now = Math.floor(Date.now() / 1000);
        return { from: now - S.hours * 3600, to: now };
    }

    async function fetchRange(range, interval, ctx) {
        const params = new URLSearchParams({
            from: String(range.from), to: String(range.to), interval: String(interval), ctx
        });
        const res = await fetch('/api/history?' + params.toString());
        const json = await res.json();
        // Cadence d'enregistrement annoncée par le firmware : cale l'auto-refresh.
        if (typeof json.measurement_interval_s === 'number' && json.measurement_interval_s > 0) {
            measurementIntervalMs = json.measurement_interval_s * 1000;
        }
        return Array.isArray(json.data) ? json.data : [];
    }

    // Une grandeur d'une source -> série filtrée + points écartés (pic isolé).
    function toSeries(rows, key, bucket) {
        const ts = rows.map((r) => r.t);
        const raw = rows.map((r) => r[key]);
        const v = filterOutliers(raw, OUTLIER_FLOOR[key]);
        const suspects = [];
        raw.forEach((x, i) => { if (x !== null && x !== undefined && v[i] === null) suspects.push([ts[i], x, 'pic']); });
        return { ts, v, bucket_s: bucket, suspects };
    }

    let drawSeq = 0; // ignore la réponse d'un dessin périmé (filtre changé entre-temps)
    async function draw() {
        const charts = $('#charts');
        const metrics = METRIC_KEYS.filter((k) => S.metrics.indexOf(k) >= 0); // ordre stable
        if (!metrics.length) {
            charts.innerHTML = '<p class="muted">Cochez au moins une grandeur à afficher.</p>';
            return;
        }
        const seq = ++drawSeq;
        const range = currentRange();
        const interval = computeInterval(range.to - range.from);
        showChartLoading(true);
        try {
            // Requêtes SÉQUENTIELLES : l'ESP32 sert son historique depuis la carte SD
            // sur un seul fil ; deux flux concurrents pouvaient faire échouer une réponse.
            const rows = {};
            for (const c of ctxsOf()) rows[c] = await fetchRange(range, interval, c);
            if (seq !== drawSeq) return;
            const D = {};
            metrics.forEach((k) => {
                D[k] = {};
                ['out', 'in'].forEach((c) => {
                    D[k][c] = rows[c] ? toSeries(rows[c], k, interval) : { ts: [], v: [], bucket_s: 0, suspects: [] };
                });
            });
            charts.innerHTML = metrics.length === 1 ? renderSingle(D, metrics[0]) : renderAll(D, metrics);
            attachHover();

            // Synthèse : sur les trois grandeurs, indépendamment de la sélection affichée.
            const filt = (c) => (rows[c] ? {
                temp: toSeries(rows[c], 'temp', interval).v,
                hum: toSeries(rows[c], 'hum', interval).v,
                pres: toSeries(rows[c], 'pres', interval).v
            } : null);
            longtermFilteredA = filt(S.source === 'in' ? 'in' : 'out');
            longtermFilteredB = S.source === 'both' ? filt('in') : null;
            updateSynthesis();
        } catch (e) {
            console.error('Erreur historique', e);
            if (seq === drawSeq) charts.innerHTML = '<p class="muted">Erreur lors de la récupération de l\'historique.</p>';
        } finally {
            if (seq === drawSeq) {
                showChartLoading(false);
                // Re-cale l'auto-refresh sur la cadence apprise via /api/history.
                scheduleAutoRefresh();
            }
        }
    }

    // Rafraîchissement automatique : seulement tant que la fenêtre touche le présent
    // et reste courte (chaque rafraîchissement relit la carte SD).
    function scheduleAutoRefresh() {
        if (longtermRefreshTimer) { clearInterval(longtermRefreshTimer); longtermRefreshTimer = null; }
        const auto = $('#autoRefreshToggle');
        if (auto && !auto.checked) return;
        if (S.range && S.range.to <= Date.now() / 1000 - STEP_S) return;
        const r = currentRange();
        if (r.to - r.from > LONGTERM_AUTOREFRESH_MAX_SECONDS) return;
        // Rythme = cadence d'enregistrement + marge : les métriques ne changent qu'à
        // chaque nouvelle mesure (~5 min).
        longtermRefreshTimer = setInterval(draw, measurementIntervalMs + LONGTERM_REFRESH_MARGIN_MS);
    }

    // --- Contrôles ------------------------------------------------------------
    function renderMetricSel() {
        $('#metricsel').innerHTML = METRICS.map((m) => {
            const on = S.metrics.indexOf(m[0]) >= 0;
            return '<label class="mk' + (on ? ' on' : '') + '"><input type="checkbox" data-m="' + m[0] + '"' + (on ? ' checked' : '') + '>' + m[1] + '</label>';
        }).join('');
    }
    renderMetricSel();
    $('#sourceCtx').innerHTML = SOURCES.map((s) => '<option value="' + s[0] + '"' + (s[0] === S.source ? ' selected' : '') + '>' + s[1] + '</option>').join('');
    $('#periods').innerHTML = PERIODS.map((p) => '<button type="button" class="pbtn" data-h="' + p[1] + '">' + p[0] + '</button>').join('') +
        '<button type="button" class="pbtn" data-h="custom">Période libre</button>';

    // Les champs datetime-local travaillent en heure LOCALE sans fuseau : conversion
    // explicite dans les deux sens, jamais via toISOString() (UTC, décalerait l'affichage).
    const floor5 = (ts) => Math.floor(ts / STEP_S) * STEP_S;
    function toLocalInput(ts) {
        const d = new Date(ts * 1000);
        const p = (n) => String(n).padStart(2, '0');
        return d.getFullYear() + '-' + p(d.getMonth() + 1) + '-' + p(d.getDate()) + 'T' + p(d.getHours()) + ':' + p(d.getMinutes());
    }
    function fromLocalInput(v) {
        if (!v) return NaN;
        const d = new Date(v);
        return isNaN(d) ? NaN : Math.floor(d.getTime() / 1000);
    }
    function fmtRange(r) {
        const o = { weekday: 'short', day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit' };
        return new Date(r.from * 1000).toLocaleString('fr-FR', o) + ' → ' + new Date(r.to * 1000).toLocaleString('fr-FR', o);
    }
    // Pré-remplit les champs : la période libre en cours, sinon la fenêtre glissante affichée.
    function fillCustom() {
        const now = floor5(Date.now() / 1000) + STEP_S;
        const r = S.range || { from: floor5(now - S.hours * 3600), to: now };
        $('#cfrom').value = toLocalInput(r.from);
        $('#cto').value = toLocalInput(r.to);
        $('#cfrom').max = $('#cto').max = toLocalInput(now);
        $('#cerr').textContent = '';
    }
    function syncPeriodUI() {
        document.querySelectorAll('#periods .pbtn').forEach((x) => x.classList.toggle('on',
            S.range ? x.dataset.h === 'custom' : +x.dataset.h === S.hours));
        $('#rangelbl').textContent = S.range ? 'Période affichée : ' + fmtRange(S.range) : '';
    }
    function applyCustom() {
        const f = fromLocalInput($('#cfrom').value), t = fromLocalInput($('#cto').value);
        const err = $('#cerr');
        if (!isFinite(f) || !isFinite(t)) { err.textContent = 'Renseignez le jour et l\'heure de début et de fin.'; return; }
        const from = floor5(f), to = floor5(t);
        if (to - from < STEP_S) { err.textContent = 'La fin doit être au moins 5 min après le début.'; return; }
        if (to - from > MAX_SPAN_S) { err.textContent = 'La période est limitée à un an.'; return; }
        err.textContent = '';
        S.range = { from, to };
        lsSet('range', JSON.stringify(S.range));
        syncPeriodUI();
        draw();
    }
    if (S.range) { $('#custom').hidden = false; fillCustom(); }
    syncPeriodUI();

    $('#metricsel').addEventListener('change', (e) => {
        if (!e.target.closest('input[data-m]')) return;
        S.metrics = METRIC_KEYS.filter((k) => {
            const el = document.querySelector('#metricsel input[data-m="' + k + '"]');
            return el && el.checked;
        });
        lsSet('metrics', JSON.stringify(S.metrics));
        renderMetricSel();
        draw();
    });
    $('#sourceCtx').addEventListener('change', (e) => { S.source = e.target.value; lsSet('source', S.source); draw(); });
    $('#showsusp').checked = S.showSuspects;
    $('#showsusp').addEventListener('change', (e) => { S.showSuspects = e.target.checked; lsSet('susp', S.showSuspects ? '1' : '0'); draw(); });
    $('#periods').addEventListener('click', (e) => {
        const b = e.target.closest('.pbtn');
        if (!b) return;
        if (b.dataset.h === 'custom') {
            // Ouvre (ou referme) le choix des bornes ; rien n'est redessiné avant « Afficher ».
            const c = $('#custom');
            c.hidden = !c.hidden;
            if (!c.hidden) fillCustom();
            return;
        }
        // Retour à une période glissante : la période libre est oubliée.
        S.hours = +b.dataset.h; S.range = null;
        lsSet('hours', S.hours); lsDel('range');
        $('#custom').hidden = true;
        syncPeriodUI();
        draw();
    });
    $('#capply').addEventListener('click', applyCustom);
    $('#custom').addEventListener('keydown', (e) => { if (e.key === 'Enter') applyCustom(); });
    $('#cnow').addEventListener('click', () => { $('#cto').value = toLocalInput(floor5(Date.now() / 1000) + STEP_S); applyCustom(); });
    // Synthèse : (re)dessinée à partir des derniers points, sans requête.
    $('#synthToggle').addEventListener('change', updateSynthesis);
    $('#autoRefreshToggle').addEventListener('change', scheduleAutoRefresh);

    draw();
}

const SYNTH_METRICS = [
    { key: 'temp', title: 'Température', unit: '°C', decimals: 1, eps: 0.1 },
    { key: 'hum', title: 'Humidité', unit: '%', decimals: 0, eps: 0.5 },
    { key: 'pres', title: 'Pression', unit: 'hPa', decimals: 1, eps: 0.1 }
];

// Calcule min / max / moyenne / variation (dernier - premier) sur des valeurs
// (les null, points aberrants écartés ou tranches vides, sont ignorés). Les
// statistiques portent donc, comme le graphe, sur les seules mesures valides.
function computeSynthesisFromValues(values) {
    const v = (values || []).filter((x) => x !== null && x !== undefined && !Number.isNaN(x));
    if (v.length === 0) return null;
    let min = v[0], max = v[0], sum = 0;
    for (const x of v) {
        if (x < min) min = x;
        if (x > max) max = x;
        sum += x;
    }
    return { min, max, avg: sum / v.length, delta: v[v.length - 1] - v[0] };
}

// Produit un objet {temp,hum,pres} de stats à partir de séries filtrées.
function buildSynthStats(filtered) {
    const out = {};
    for (const m of SYNTH_METRICS) {
        out[m.key] = filtered ? computeSynthesisFromValues(filtered[m.key]) : null;
    }
    return out;
}

// Affiche la synthèse à partir de deux jeux de stats déjà calculés (A et B).
function renderSynthesisStats(statsA, statsB) {
    const panel = document.getElementById('synthPanel');
    const toggle = document.getElementById('synthToggle');
    if (!panel) return;

    if (!toggle || !toggle.checked || !statsA) {
        panel.hidden = true;
        panel.innerHTML = '';
        return;
    }

    const nf = (v, decimals) => v.toLocaleString('fr-FR', {
        minimumFractionDigits: decimals,
        maximumFractionDigits: decimals
    });
    const arrow = (delta, eps) => {
        if (delta > eps) return { symbol: '▲', cls: 'up' };
        if (delta < -eps) return { symbol: '▼', cls: 'down' };
        return { symbol: '=', cls: 'flat' };
    };

    const blocks = SYNTH_METRICS.map((m) => {
        const s = statsA[m.key];
        if (!s) {
            return `<div class="synth-metric"><h3>${m.title}</h3><div class="synth-delta flat">N/D</div></div>`;
        }
        const a = arrow(s.delta, m.eps);
        const sign = s.delta > 0 ? '+' : '';

        // Vue IN+OUT : écart des moyennes extérieur − intérieur (positif = dehors
        // plus élevé que dedans). statsA = OUT, statsB = IN.
        let compareLine = '';
        const sB = statsB ? statsB[m.key] : null;
        if (sB) {
            const diff = s.avg - sB.avg;
            const da = arrow(diff, m.eps);
            const dsign = diff > 0 ? '+' : '';
            compareLine = `<div class="synth-compare ${da.cls}">OUT − IN (moy.) : ${da.symbol} ${dsign}${nf(diff, m.decimals)} ${m.unit}</div>`;
        }

        return `
            <div class="synth-metric">
                <h3>${m.title}</h3>
                <div class="synth-delta ${a.cls}">${a.symbol} ${sign}${nf(s.delta, m.decimals)} ${m.unit}</div>
                <div class="synth-stats">
                    <span>Min : ${nf(s.min, m.decimals)} ${m.unit}</span>
                    <span>Max : ${nf(s.max, m.decimals)} ${m.unit}</span>
                    <span>Moyenne : ${nf(s.avg, m.decimals)} ${m.unit}</span>
                </div>
                ${compareLine}
            </div>`;
    });

    panel.innerHTML = blocks.join('');
    panel.hidden = false;
}

// Met à jour la synthèse à partir des séries filtrées (valeurs aberrantes exclues),
// pour que les statistiques restent représentatives comme le graphe.
function updateSynthesis() {
    const panel = document.getElementById('synthPanel');
    const toggle = document.getElementById('synthToggle');
    if (!panel) return;
    if (!toggle || !toggle.checked || !longtermFilteredA) {
        panel.hidden = true;
        panel.innerHTML = '';
        return;
    }
    const statsA = buildSynthStats(longtermFilteredA);
    const statsB = longtermFilteredB ? buildSynthStats(longtermFilteredB) : null;
    renderSynthesisStats(statsA, statsB);
}

window.onload = () => {
    initAlertModal();
    fetchSystem();
    fetchLive();
    fetchAlert();

    if (getPageName() === 'longterm') initHistoryPage();

    if (isStatsPage()) {
        fetchStats();
        initStatsControls();
    }

    setInterval(fetchLive, LIVE_REFRESH_MS);
    setInterval(fetchAlert, ALERT_REFRESH_MS);
};
