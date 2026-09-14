// Affichage de l'alerte météo sur le dashboard
let current_alert_payload = null;

// Injection dynamique des valeurs min/max via graph_config.js
// (graph_config.js est généré lors du build à partir de config.h)
let GRAPH_SCALE_MODE = window.GRAPH_CONFIG?.scale_mode ?? 2;
let GRAPH_SCALE_MARGIN_PCT = window.GRAPH_CONFIG?.scale_margin_pct ?? 20;
const GRAPH_TEMP_MIN = window.GRAPH_CONFIG?.temp_min ?? -10.0;
const GRAPH_TEMP_MAX = window.GRAPH_CONFIG?.temp_max ?? 40.0;
const GRAPH_HUM_MIN  = window.GRAPH_CONFIG?.hum_min  ?? 20.0;
const GRAPH_HUM_MAX  = window.GRAPH_CONFIG?.hum_max  ?? 90.0;
const GRAPH_PRES_MIN = window.GRAPH_CONFIG?.pres_min ?? 970.0;
const GRAPH_PRES_MAX = window.GRAPH_CONFIG?.pres_max ?? 1040.0;

// Ajout UI : contrôle du mode d'échelle et du pourcentage
function setGraphScaleMode(mode) {
    GRAPH_SCALE_MODE = mode;
    updateChartScale();
}
function setGraphScaleMarginPct(pct) {
    GRAPH_SCALE_MARGIN_PCT = pct;
    document.getElementById('scaleMarginValue').textContent = pct;
    updateChartScale();
}

function getDynamicMinMax(data, key, userMin, userMax) {
    const values = data.map(d => d[key]).filter(v => v !== null && v !== undefined && !Number.isNaN(v));
    if (values.length === 0) return [userMin, userMax];
    let dynMin = Math.min(...values);
    let dynMax = Math.max(...values);
    if (dynMin === dynMax) {
        dynMin -= 0.3;
        dynMax += 0.3;
    }
    if (GRAPH_SCALE_MODE === 1) {
        // Dynamique : l'échelle épouse exactement l'amplitude des données.
        return [dynMin, dynMax];
    } else if (GRAPH_SCALE_MODE === 2) {
        // Mixte : le curseur « Zoom » interpole entre l'échelle complète (0 %,
        // min/max fixes → la courbe apparaît quasiment plate/unique) et l'amplitude
        // exacte des données (100 % → la courbe occupe toute la hauteur).
        const f = Math.min(Math.max(GRAPH_SCALE_MARGIN_PCT / 100, 0), 1);
        const min = userMin + (dynMin - userMin) * f;
        const max = userMax + (dynMax - userMax) * f;
        return [min, max];
    } else {
        // Fixe : min/max configurés (équivalent à un zoom de 0 %).
        return [userMin, userMax];
    }
}

function updateChartScale() {
    if (!chart || !chart.data || !chart.data.datasets) return;

    // Regroupe toutes les valeurs par axe (y=temp, y1=hum, y2=pres), en tenant
    // compte des éventuels jeux de données de comparaison (période B).
    const collect = (axisId) => {
        const out = [];
        chart.data.datasets.forEach((ds) => {
            if (ds.yAxisID !== axisId || !Array.isArray(ds.data)) return;
            ds.data.forEach((v) => out.push({ v }));
        });
        return out;
    };

    const tempData = collect('y');
    if (tempData.length === 0) return;
    const humData = collect('y1');
    const presData = collect('y2');

    const [tmin, tmax] = getDynamicMinMax(tempData, 'v', GRAPH_TEMP_MIN, GRAPH_TEMP_MAX);
    const [hmin, hmax] = getDynamicMinMax(humData, 'v', GRAPH_HUM_MIN, GRAPH_HUM_MAX);
    const [pmin, pmax] = getDynamicMinMax(presData, 'v', GRAPH_PRES_MIN, GRAPH_PRES_MAX);
    chart.options.scales.y.min = tmin;
    chart.options.scales.y.max = tmax;
    chart.options.scales.y1.min = hmin;
    chart.options.scales.y1.max = hmax;
    chart.options.scales.y2.min = pmin;
    chart.options.scales.y2.max = pmax;
    chart.update();
}

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
let chart;

const HISTORY_WINDOWS_SECONDS = {
    short: 2 * 60 * 60,
    long: 24 * 60 * 60
};

const HISTORY_REFRESH_MS = 15000;
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

function isHistoryPage() {
    // Seule la page Historique porte désormais un graphe (l'accueil n'en a plus).
    return getPageName() === 'longterm';
}

function isStatsPage() {
    return getPageName() === 'stats';
}

function getHistoryWindowSeconds() {
    return getPageName() === 'longterm' ? HISTORY_WINDOWS_SECONDS.long : HISTORY_WINDOWS_SECONDS.short;
}

function getHistoryIntervalSeconds() {
    return getPageName() === 'longterm' ? 30 * 60 : 5 * 60;
}

function buildHistoryUrl() {
    const params = new URLSearchParams({
        window: String(getHistoryWindowSeconds()),
        interval: String(getHistoryIntervalSeconds())
    });
    return `/api/history?${params.toString()}`;
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

    // Batterie de la sonde déportée + alerte pile faible.
    const batteryRow = document.getElementById('outBatteryRow');
    const batteryAlert = document.getElementById('outBatteryAlert');
    if (batteryRow && batteryAlert) {
        if (outb.has_battery) {
            batteryRow.hidden = false;
            setText('outBattery', Number(outb.battery_pct).toFixed(0));
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

async function fetchHistory() {
    if (!chart || !isHistoryPage()) return;

    try {
        const res = await fetch(buildHistoryUrl());
        const json = await res.json();
        updateChart(Array.isArray(json.data) ? json.data : []);
    } catch (e) {
        console.error('Erreur historique', e);
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

function updateChart(data) {
    if (!chart) return;
    chart.data.labels = data.map((d) => new Date(d.t * 1000).toLocaleTimeString());
    chart.data.datasets[0].data = filterOutliers(data.map((d) => d.temp), OUTLIER_FLOOR.temp);
    chart.data.datasets[1].data = filterOutliers(data.map((d) => d.hum), OUTLIER_FLOOR.hum);
    chart.data.datasets[2].data = filterOutliers(data.map((d) => d.pres), OUTLIER_FLOOR.pres);
    updateChartScale();
}

function initChart() {
    const chart_canvas = document.getElementById('historyChart');
    if (!chart_canvas || !isHistoryPage()) return;

    const ctx = chart_canvas.getContext('2d');
    chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: '°C',
                    data: [],
                    borderColor: '#00a8ff',
                    backgroundColor: 'rgba(0, 168, 255, 0.1)',
                    tension: 0.45,
                    cubicInterpolationMode: 'monotone',
                    pointRadius: 0,
                    stepped: false,
                    yAxisID: 'y'
                },
                {
                    label: 'Hu%',
                    data: [],
                    borderColor: '#00ff88',
                    backgroundColor: 'rgba(0, 255, 136, 0.1)',
                    tension: 0.45,
                    cubicInterpolationMode: 'monotone',
                    pointRadius: 0,
                    stepped: false,
                    yAxisID: 'y1',
                    hidden: false
                },
                {
                    label: 'hPa',
                    data: [],
                    borderColor: '#ff00ff',
                    backgroundColor: 'rgba(255, 0, 255, 0.1)',
                    tension: 0.45,
                    cubicInterpolationMode: 'monotone',
                    pointRadius: 0,
                    stepped: false,
                    yAxisID: 'y2',
                    hidden: false
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            spanGaps: true, // relie par-dessus les points écartés (valeurs aberrantes -> null)
            interaction: { mode: 'index', intersect: false },
            scales: {
                y: {
                    type: 'linear',
                    display: true,
                    position: 'left',
                    title: { display: true, text: 'Temp (°C)', color: '#00a8ff' },
                    ticks: { color: '#00a8ff' }
                },
                y1: {
                    type: 'linear',
                    display: true,
                    position: 'right',
                    grid: { drawOnChartArea: false },
                    title: { display: true, text: 'Hum (%)', color: '#00ff88' },
                    ticks: { color: '#00ff88' }
                },
                y2: {
                    type: 'linear',
                    display: true,
                    position: 'right',
                    grid: { drawOnChartArea: false },
                    title: { display: true, text: 'Pres (hPa)', color: '#ff00ff' },
                    ticks: { color: '#ff00ff' }
                },
                x: {
                    // Axe catégoriel (labels = horodatages formatés selon la durée
                    // choisie, cf. formatTsLabel). autoSkip + maxTicksLimit gardent
                    // une graduation lisible quelle que soit la période (quelques
                    // minutes comme 30 jours), sans rotation.
                    ticks: {
                        color: '#9aa4b2',
                        autoSkip: true,
                        maxTicksLimit: 10,
                        maxRotation: 0,
                        minRotation: 0
                    }
                }
            }
        }
    });
    updateChartScale();
}

// ------------------------------------------------------------------
// Page Historique : sélection et comparaison de périodes arbitraires
// ------------------------------------------------------------------
const LONGTERM_TARGET_POINTS = 250; // points visés/requête (marge sous la limite ESP32)
let longtermRefreshTimer = null;

// Calcule un intervalle d'agrégation (secondes) pour tenir ~LONGTERM_TARGET_POINTS.
function computeInterval(durationSeconds) {
    let interval = Math.floor(durationSeconds / LONGTERM_TARGET_POINTS);
    if (interval < 60) interval = 60; // pas de tranche plus fine qu'une minute
    return interval;
}

// Formate un horodatage (secondes Unix) pour l'axe X, en adaptant la précision à
// la DURÉE affichée : heure:minute pour une plage courte (quelques minutes à 24 h),
// jour/mois + heure pour quelques jours, jour/mois seul pour un mois. L'axe reflète
// ainsi la période demandée au lieu d'un format fixe.
function formatTsLabel(tsSeconds, spanSeconds) {
    const d = new Date(tsSeconds * 1000);
    const p = (n) => String(n).padStart(2, '0');
    if (spanSeconds <= 24 * 3600) {
        return `${p(d.getHours())}:${p(d.getMinutes())}`;
    }
    if (spanSeconds <= 7 * 24 * 3600) {
        return `${p(d.getDate())}/${p(d.getMonth() + 1)} ${p(d.getHours())}:${p(d.getMinutes())}`;
    }
    return `${p(d.getDate())}/${p(d.getMonth() + 1)}`;
}

// Convertit la valeur d'un <input type="datetime-local"> (heure locale) en secondes Unix.
function localInputToUnix(value) {
    if (!value) return null;
    const ms = new Date(value).getTime();
    if (Number.isNaN(ms)) return null;
    return Math.floor(ms / 1000);
}

// Convertit des secondes Unix vers le format attendu par <input type="datetime-local">.
function unixToLocalInput(unix) {
    const d = new Date(unix * 1000);
    const pad = (n) => String(n).padStart(2, '0');
    return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}T${pad(d.getHours())}:${pad(d.getMinutes())}`;
}

// Détermine la période principale (A) à partir des contrôles.
function getPrimaryRange() {
    const preset = document.getElementById('periodPreset')?.value || '86400';
    const now = Math.floor(Date.now() / 1000);
    if (preset === 'custom') {
        const from = localInputToUnix(document.getElementById('fromInput')?.value);
        const to = localInputToUnix(document.getElementById('toInput')?.value);
        if (from && to && to > from) return { from, to, relative: false };
        return null;
    }
    if (preset === 'today') {
        const d = new Date();
        d.setHours(0, 0, 0, 0);
        return { from: Math.floor(d.getTime() / 1000), to: now, relative: true };
    }
    const seconds = Number(preset);
    return { from: now - seconds, to: now, relative: true };
}

// Source sélectionnée sur la page Historique : OUT (météo extérieure, défaut), IN
// (confort intérieur) ou BOTH (les deux, tracées ensemble). Propagé aux requêtes.
function getSourceCtx() {
    return document.getElementById('sourceCtx')?.value || 'out';
}

function sourceLabel() {
    const c = getSourceCtx();
    if (c === 'in') return 'Intérieur';
    if (c === 'both') return 'Intérieur + Extérieur';
    return 'Extérieur';
}

async function fetchRange(range, interval, ctxOverride) {
    const params = new URLSearchParams({
        from: String(range.from),
        to: String(range.to),
        interval: String(interval),
        ctx: ctxOverride || getSourceCtx()
    });
    const res = await fetch(`/api/history?${params.toString()}`);
    const json = await res.json();
    // Cadence d'enregistrement annoncée par le firmware : cale l'auto-refresh.
    if (typeof json.measurement_interval_s === 'number' && json.measurement_interval_s > 0) {
        measurementIntervalMs = json.measurement_interval_s * 1000;
    }
    return Array.isArray(json.data) ? json.data : [];
}

// Vue OUT : comble les tranches sans mesure extérieure par l'intérieur, EN LES
// MARQUANT (jamais un point IN présenté comme OUT). Modifie outData en place et
// renvoie, par grandeur, un tableau de drapeaux "comblé" aligné sur les points,
// plus le nombre de tranches comblées.
function fillOutWithIn(outData, inData) {
    const flags = { temp: [], hum: [], pres: [], slices: 0 };
    const keys = ['temp', 'hum', 'pres'];
    for (let i = 0; i < outData.length; i++) {
        const o = outData[i];
        const inp = inData[i] || {};
        let filledHere = false;
        for (const k of keys) {
            const ov = o[k];
            const iv = inp[k];
            if (ov === null || ov === undefined) {
                if (iv !== null && iv !== undefined) {
                    o[k] = iv;           // secours intérieur
                    flags[k].push(true); // marqué comme comblé
                    filledHere = true;
                } else {
                    flags[k].push(false);
                }
            } else {
                flags[k].push(false);
            }
        }
        if (filledHere) flags.slices++;
    }
    return flags;
}

// Couleur des points comblés (IN), cohérente avec le badge "secours intérieur".
const FILL_COLOR = '#ff9f2e';

// Applique le marquage visuel : les points comblés (IN) ressortent en pastilles
// oranges ; les points OUT restent une simple ligne (rayon 0). Cas particulier :
// un point VALIDE ISOLÉ (ses deux voisins sont nuls) doit afficher un marqueur,
// sinon une ligne ne peut rien tracer et le point est invisible : c'est ce qui
// arrivait avec une seule mesure (juste après un reset) : graphe vide alors que
// la donnée existe. On donne donc un petit rayon à ces points isolés.
function applyFillStyle(dataset, flags) {
    const data = dataset.data || [];
    const isNum = (v) => typeof v === 'number' && isFinite(v);
    const isolated = (i) => isNum(data[i]) && !isNum(data[i - 1]) && !isNum(data[i + 1]);
    if (!flags) {
        dataset.pointRadius = data.map((_, i) => (isolated(i) ? 2.5 : 0));
        dataset.pointBackgroundColor = dataset.borderColor;
        return;
    }
    dataset.pointRadius = data.map((_, i) => (flags[i] ? 3 : (isolated(i) ? 2.5 : 0)));
    dataset.pointBackgroundColor = flags.map((f) => (f ? FILL_COLOR : dataset.borderColor));
}

function formatRangeLabel(range) {
    const opts = { dateStyle: 'short', timeStyle: 'short' };
    const f = new Date(range.from * 1000).toLocaleString('fr-FR', opts);
    const t = new Date(range.to * 1000).toLocaleString('fr-FR', opts);
    return `${f} → ${t}`;
}

// Deuxième source (l'INTÉRIEUR en vue IN+OUT) : mêmes couleurs et mêmes axes que
// les courbes principales (OUT), différenciée uniquement par un tracé en
// pointillés. Alignée par index sur la source principale (même plage/intervalle).
const SECOND_COLORS = { temp: '#00a8ff', hum: '#00ff88', pres: '#ff00ff' };

// Ajoute (ou retire) les 3 jeux de données de la 2e source. `filtered` est un
// objet {temp,hum,pres} de tableaux déjà filtrés (l'INTÉRIEUR), ou null.
function setSecondaryDatasets(filtered) {
    chart.data.datasets = chart.data.datasets.slice(0, 3); // conserve la source principale
    if (!filtered) return;
    const mk = (label, key, color, axis) => ({
        label,
        data: filtered[key],
        borderColor: color,
        backgroundColor: 'transparent',
        borderDash: [6, 4],
        tension: 0.45,
        cubicInterpolationMode: 'monotone',
        pointRadius: 0,
        yAxisID: axis
    });
    chart.data.datasets.push(mk('°C IN', 'temp', SECOND_COLORS.temp, 'y'));
    chart.data.datasets.push(mk('Hu% IN', 'hum', SECOND_COLORS.hum, 'y1'));
    chart.data.datasets.push(mk('hPa IN', 'pres', SECOND_COLORS.pres, 'y2'));
}

// Construit les 3 séries filtrées (valeurs aberrantes -> null) d'un jeu de points.
function buildFilteredSeries(data) {
    if (!Array.isArray(data)) return null;
    return {
        temp: filterOutliers(data.map((d) => d.temp), OUTLIER_FLOOR.temp),
        hum: filterOutliers(data.map((d) => d.hum), OUTLIER_FLOOR.hum),
        pres: filterOutliers(data.map((d) => d.pres), OUTLIER_FLOOR.pres)
    };
}

function showChartLoading(visible) {
    const el = document.getElementById('chartLoading');
    if (el) el.hidden = !visible;
}

// Séries filtrées des périodes A et B conservées pour (re)calculer la synthèse
// (activation de la bascule) sans relancer le chargement du graphe.
let longtermFilteredA = null;
let longtermFilteredB = null;

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

async function refreshLongterm() {
    if (!chart) return;
    const info = document.getElementById('periodInfo');
    const primary = getPrimaryRange();
    if (!primary) {
        if (info) info.textContent = 'Sélectionnez une période valide (le début doit précéder la fin).';
        return;
    }
    const interval = computeInterval(primary.to - primary.from);
    const spanSeconds = primary.to - primary.from;

    showChartLoading(true);
    try {
        const ctx = getSourceCtx(); // 'out' | 'in' | 'both'

        if (ctx === 'both') {
            // Intérieur + Extérieur : deux séries, même axe temporel. OUT en trait
            // plein (source principale), IN en pointillés. Requêtes SÉQUENTIELLES :
            // l'ESP32 sert son historique depuis la carte SD sur un seul fil ; deux
            // flux concurrents pouvaient faire échouer une réponse (plage vide alors
            // que les données existent). Même plage/intervalle -> points alignés.
            const outData = await fetchRange(primary, interval, 'out');
            const inData  = await fetchRange(primary, interval, 'in');
            const base = outData.length >= inData.length ? outData : inData;
            chart.data.labels = base.map((d) => formatTsLabel(d.t, spanSeconds));

            const fOut = buildFilteredSeries(outData) || { temp: [], hum: [], pres: [] };
            const fIn  = buildFilteredSeries(inData)  || { temp: [], hum: [], pres: [] };
            chart.data.datasets[0].data = fOut.temp;
            chart.data.datasets[1].data = fOut.hum;
            chart.data.datasets[2].data = fOut.pres;
            setSecondaryDatasets(fIn); // IN en pointillés (datasets 3-5)
            // Points isolés visibles sur la source principale (pas de comblement ici).
            for (let i = 0; i < 3; i++) applyFillStyle(chart.data.datasets[i], null);

            updateChartScale();
            longtermFilteredA = fOut; // OUT
            longtermFilteredB = fIn;  // IN
            updateSynthesis();
            if (info) info.textContent =
                `Intérieur + Extérieur · Période : ${formatRangeLabel(primary)}`;
        } else {
            // Source unique. En vue OUT, comble les tranches sans mesure extérieure
            // par l'intérieur, EN LES MARQUANT (jamais un point IN présenté comme OUT).
            const needIn = (ctx === 'out');
            const data = await fetchRange(primary, interval, ctx);
            const inFill = needIn ? await fetchRange(primary, interval, 'in') : null;
            let fillFlags = null;
            if (needIn && inFill) fillFlags = fillOutWithIn(data, inFill);

            chart.data.labels = data.map((d) => formatTsLabel(d.t, spanSeconds));
            const filtered = buildFilteredSeries(data) || { temp: [], hum: [], pres: [] };
            chart.data.datasets[0].data = filtered.temp;
            chart.data.datasets[1].data = filtered.hum;
            chart.data.datasets[2].data = filtered.pres;
            setSecondaryDatasets(null); // pas de 2e source

            applyFillStyle(chart.data.datasets[0], fillFlags ? fillFlags.temp : null);
            applyFillStyle(chart.data.datasets[1], fillFlags ? fillFlags.hum : null);
            applyFillStyle(chart.data.datasets[2], fillFlags ? fillFlags.pres : null);

            updateChartScale();
            longtermFilteredA = filtered;
            longtermFilteredB = null;
            updateSynthesis();
            if (info) {
                const filledNote = (fillFlags && fillFlags.slices > 0)
                    ? `   •   ${fillFlags.slices} tranche(s) comblée(s) par l'intérieur (en orange)`
                    : '';
                info.textContent = `${sourceLabel()} · Période : ${formatRangeLabel(primary)}` + filledNote;
            }
        }
    } catch (e) {
        console.error('Erreur historique', e);
        if (info) info.textContent = 'Erreur lors de la récupération de l’historique.';
    } finally {
        showChartLoading(false);
        // Re-cale l'auto-refresh sur la cadence apprise via /api/history
        // (measurement_interval_s), au cas où elle diffère du défaut.
        scheduleLongtermAutoRefresh();
    }
}

// Durée maximale d'une période encore rafraîchie automatiquement (48 h). Au-delà,
// chaque rafraîchissement relancerait un scan de plusieurs fichiers CSV sur la carte
// SD : on évite de le répéter en continu (l'utilisateur peut recharger manuellement).
const LONGTERM_AUTOREFRESH_MAX_SECONDS = 172800;

// (Re)programme le rafraîchissement automatique : uniquement pour une période
// relative (qui suit « maintenant »), sans comparaison et de courte durée.
function scheduleLongtermAutoRefresh() {
    if (longtermRefreshTimer) { clearInterval(longtermRefreshTimer); longtermRefreshTimer = null; }
    const autoToggle = document.getElementById('autoRefreshToggle');
    if (autoToggle && !autoToggle.checked) return; // mise à jour temps réel désactivée
    const preset = document.getElementById('periodPreset')?.value || '86400';
    if (preset === 'custom') return;
    const primary = getPrimaryRange();
    if (!primary) return;
    if ((primary.to - primary.from) > LONGTERM_AUTOREFRESH_MAX_SECONDS) return;
    // Rythme = cadence d'enregistrement + marge (au lieu d'un intervalle court) :
    // les métriques ne changent qu'à chaque nouvelle mesure (~5 min).
    longtermRefreshTimer = setInterval(refreshLongterm,
                                       measurementIntervalMs + LONGTERM_REFRESH_MARGIN_MS);
}

function initLongtermControls() {
    const periodPreset = document.getElementById('periodPreset');
    const customRange = document.getElementById('customRange');
    const fromInput = document.getElementById('fromInput');
    const toInput = document.getElementById('toInput');

    const syncVisibility = () => {
        if (customRange) customRange.hidden = periodPreset?.value !== 'custom';
        scheduleLongtermAutoRefresh();
    };

    // Applique automatiquement toute modification des sélecteurs / champs de dates.
    const applyNow = () => { syncVisibility(); refreshLongterm(); };

    if (periodPreset) periodPreset.addEventListener('change', () => {
        // Pré-remplit les champs personnalisés avec la dernière plage 24 h.
        if (periodPreset.value === 'custom') {
            const now = Math.floor(Date.now() / 1000);
            if (fromInput && !fromInput.value) fromInput.value = unixToLocalInput(now - 86400);
            if (toInput && !toInput.value) toInput.value = unixToLocalInput(now);
        }
        applyNow();
    });
    const sourceCtx = document.getElementById('sourceCtx');
    if (sourceCtx) sourceCtx.addEventListener('change', refreshLongterm);
    if (fromInput) fromInput.addEventListener('change', refreshLongterm);
    if (toInput) toInput.addEventListener('change', refreshLongterm);

    // La bascule Synthèse (re)dessine à partir des derniers points, sans requête.
    const synthToggle = document.getElementById('synthToggle');
    if (synthToggle) synthToggle.addEventListener('change', updateSynthesis);

    // La bascule « Temps réel » (dé)active le rafraîchissement automatique.
    const autoToggle = document.getElementById('autoRefreshToggle');
    if (autoToggle) autoToggle.addEventListener('change', scheduleLongtermAutoRefresh);

    syncVisibility();
    refreshLongterm();
}

window.onload = () => {
    initAlertModal();
    fetchSystem();
    fetchLive();
    fetchAlert();

    if (getPageName() === 'longterm') {
        initChart();

        // Zoom par défaut porté par l'attribut value du slider de la page.
        const marginSlider = document.getElementById('scaleMargin');
        const marginValue = document.getElementById('scaleMarginValue');
        if (marginSlider) {
            GRAPH_SCALE_MARGIN_PCT = Number(marginSlider.value);
            if (marginValue) marginValue.textContent = marginSlider.value;
        }
        const modeSelect = document.getElementById('scaleMode');
        if (modeSelect) modeSelect.value = GRAPH_SCALE_MODE;

        // Page Historique : pilotée par la sélection de période.
        initLongtermControls();
    }

    if (isStatsPage()) {
        fetchStats();
        initStatsControls();
    }

    setInterval(fetchLive, LIVE_REFRESH_MS);
    setInterval(fetchAlert, ALERT_REFRESH_MS);
};
