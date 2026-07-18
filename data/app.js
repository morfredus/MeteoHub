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
        const details = isDetailed && detailsText ? ` — ${detailsText}` : "";

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

        body.innerHTML = `<p><strong>${levelLabel} (${level})</strong> — ${event}</p><p><strong>Source :</strong> ${senderValue}</p><p><strong>${validity}</strong></p><p>${description}</p><p><strong>Consigne :</strong> Surveillez l’évolution locale et limitez les déplacements non essentiels.</p>`;
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

function getPageName() {
    return document.body?.dataset?.page || 'dashboard';
}

function isHistoryPage() {
    const page = getPageName();
    return page === 'dashboard' || page === 'longterm';
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

async function fetchLive() {
    try {
        const res = await fetch('/api/live');
        const data = await res.json();

        const temp = document.getElementById('temp');
        const hum = document.getElementById('hum');
        const pres = document.getElementById('pres');
        if (temp) temp.textContent = data.temp.toFixed(1);
        if (hum) hum.textContent = data.hum.toFixed(0);
        if (pres) pres.textContent = data.pres.toFixed(1);

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

async function fetchStats() {
    if (!isStatsPage()) return;

    try {
        const res = await fetch('/api/stats');
        const data = await res.json();
        const tbody = document.getElementById('statsBody');
        if (!tbody) return;

        tbody.innerHTML = `
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
    } catch (e) {
        console.error('Erreur stats', e);
    }
}

// Seuils de bruit négligeable par grandeur : en dessous, un écart ponctuel n'est
// jamais considéré comme aberrant (évite de « nettoyer » les micro-variations).
const OUTLIER_FLOOR = { temp: 1.0, hum: 6, pres: 1.0 };

// Détection de valeurs aberrantes fondée sur la cohérence temporelle (et non sur
// un seuil fixe) : un point est écarté (mis à null) s'il s'éloigne fortement de
// SES DEUX voisins alors que ceux-ci restent cohérents entre eux — c.-à-d. un pic
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
                }
            }
        }
    });
    updateChartScale();
}

// ------------------------------------------------------------------
// Page Historique : sélection et comparaison de périodes arbitraires
// ------------------------------------------------------------------
const LONGTERM_TARGET_POINTS = 300; // nombre de points visés par requête
let longtermRefreshTimer = null;

// Calcule un intervalle d'agrégation (secondes) pour tenir ~LONGTERM_TARGET_POINTS.
function computeInterval(durationSeconds) {
    let interval = Math.floor(durationSeconds / LONGTERM_TARGET_POINTS);
    if (interval < 60) interval = 60; // pas de tranche plus fine qu'une minute
    return interval;
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

// Détermine la période de comparaison (B), de même durée que A.
function getCompareRange(primary) {
    const mode = document.getElementById('comparePreset')?.value || 'none';
    if (mode === 'none') return null;
    const duration = primary.to - primary.from;
    if (mode === 'prev') {
        return { from: primary.from - duration, to: primary.from };
    }
    if (mode === 'custom') {
        const from = localInputToUnix(document.getElementById('compareFromInput')?.value);
        if (!from) return null;
        return { from, to: from + duration };
    }
    return null;
}

async function fetchRange(range, interval) {
    const params = new URLSearchParams({
        from: String(range.from),
        to: String(range.to),
        interval: String(interval)
    });
    const res = await fetch(`/api/history?${params.toString()}`);
    const json = await res.json();
    return Array.isArray(json.data) ? json.data : [];
}

function formatRangeLabel(range) {
    const opts = { dateStyle: 'short', timeStyle: 'short' };
    const f = new Date(range.from * 1000).toLocaleString('fr-FR', opts);
    const t = new Date(range.to * 1000).toLocaleString('fr-FR', opts);
    return `${f} → ${t}`;
}

// La période comparée B reprend les mêmes couleurs que les courbes principales,
// différenciée uniquement par un tracé en pointillés.
const COMPARE_COLORS = { temp: '#00a8ff', hum: '#00ff88', pres: '#ff00ff' };

// Ajoute (ou retire) les 3 jeux de données de la période B, alignés par index sur A.
// filteredB est un objet {temp,hum,pres} de tableaux déjà filtrés (ou null).
function setComparisonDatasets(filteredB) {
    chart.data.datasets = chart.data.datasets.slice(0, 3); // conserve A (temp/hum/pres)
    if (!filteredB) return;
    const mk = (label, key, color, axis) => ({
        label,
        data: filteredB[key],
        borderColor: color,
        backgroundColor: 'transparent',
        borderDash: [6, 4],
        tension: 0.45,
        cubicInterpolationMode: 'monotone',
        pointRadius: 0,
        yAxisID: axis
    });
    chart.data.datasets.push(mk('°C (B)', 'temp', COMPARE_COLORS.temp, 'y'));
    chart.data.datasets.push(mk('Hu% (B)', 'hum', COMPARE_COLORS.hum, 'y1'));
    chart.data.datasets.push(mk('hPa (B)', 'pres', COMPARE_COLORS.pres, 'y2'));
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
// (les null — points aberrants écartés ou tranches vides — sont ignorés). Les
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

        // En comparaison : écart des moyennes A − B (positif = A supérieure à B).
        let compareLine = '';
        const sB = statsB ? statsB[m.key] : null;
        if (sB) {
            const diff = s.avg - sB.avg;
            const da = arrow(diff, m.eps);
            const dsign = diff > 0 ? '+' : '';
            compareLine = `<div class="synth-compare ${da.cls}">A − B (moy.) : ${da.symbol} ${dsign}${nf(diff, m.decimals)} ${m.unit}</div>`;
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
    const compare = getCompareRange(primary);
    const interval = computeInterval(primary.to - primary.from);
    const spanOverDay = (primary.to - primary.from) > 86400;

    showChartLoading(true);
    try {
        const dataA = await fetchRange(primary, interval);
        const dataB = compare ? await fetchRange(compare, interval) : null;

        // Axe X : horodatage réel de la période A (les points B sont alignés par index).
        chart.data.labels = dataA.map((d) => {
            const dt = new Date(d.t * 1000);
            return spanOverDay
                ? dt.toLocaleString('fr-FR', { day: '2-digit', month: '2-digit', hour: '2-digit', minute: '2-digit' })
                : dt.toLocaleTimeString('fr-FR', { hour: '2-digit', minute: '2-digit' });
        });
        // Filtre les valeurs aberrantes (pics/creux d'un point) pour le tracé et les stats.
        const filteredA = buildFilteredSeries(dataA);
        const filteredB = dataB ? buildFilteredSeries(dataB) : null;
        chart.data.datasets[0].data = filteredA.temp;
        chart.data.datasets[1].data = filteredA.hum;
        chart.data.datasets[2].data = filteredA.pres;
        setComparisonDatasets(filteredB);
        updateChartScale();

        longtermFilteredA = filteredA;
        longtermFilteredB = filteredB;
        updateSynthesis();

        if (info) {
            info.textContent = compare
                ? `Période A : ${formatRangeLabel(primary)}   •   Période B : ${formatRangeLabel(compare)}`
                : `Période : ${formatRangeLabel(primary)}`;
        }
    } catch (e) {
        console.error('Erreur historique', e);
        if (info) info.textContent = 'Erreur lors de la récupération de l’historique.';
    } finally {
        showChartLoading(false);
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
    const preset = document.getElementById('periodPreset')?.value || '86400';
    const compareMode = document.getElementById('comparePreset')?.value || 'none';
    if (preset === 'custom' || compareMode !== 'none') return;
    const primary = getPrimaryRange();
    if (!primary) return;
    if ((primary.to - primary.from) > LONGTERM_AUTOREFRESH_MAX_SECONDS) return;
    longtermRefreshTimer = setInterval(refreshLongterm, HISTORY_REFRESH_MS);
}

function initLongtermControls() {
    const periodPreset = document.getElementById('periodPreset');
    const customRange = document.getElementById('customRange');
    const comparePreset = document.getElementById('comparePreset');
    const compareCustom = document.getElementById('compareCustom');
    const fromInput = document.getElementById('fromInput');
    const toInput = document.getElementById('toInput');
    const compareFromInput = document.getElementById('compareFromInput');

    const syncVisibility = () => {
        if (customRange) customRange.hidden = periodPreset?.value !== 'custom';
        if (compareCustom) compareCustom.hidden = comparePreset?.value !== 'custom';
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
    if (comparePreset) comparePreset.addEventListener('change', applyNow);
    if (fromInput) fromInput.addEventListener('change', refreshLongterm);
    if (toInput) toInput.addEventListener('change', refreshLongterm);
    if (compareFromInput) compareFromInput.addEventListener('change', refreshLongterm);

    // La bascule Synthèse (re)dessine à partir des derniers points, sans requête.
    const synthToggle = document.getElementById('synthToggle');
    if (synthToggle) synthToggle.addEventListener('change', updateSynthesis);

    syncVisibility();
    refreshLongterm();
}

window.onload = () => {
    initAlertModal();
    fetchSystem();
    fetchLive();
    fetchAlert();

    if (isHistoryPage()) {
        initChart();

        // La valeur par défaut du zoom est portée par l'attribut value du slider,
        // spécifique à chaque page (90 % sur le tableau de bord, 75 % sur l'historique).
        const marginSlider = document.getElementById('scaleMargin');
        const marginValue = document.getElementById('scaleMarginValue');
        if (marginSlider) {
            GRAPH_SCALE_MARGIN_PCT = Number(marginSlider.value);
            if (marginValue) marginValue.textContent = marginSlider.value;
        }
        const modeSelect = document.getElementById('scaleMode');
        if (modeSelect) modeSelect.value = GRAPH_SCALE_MODE;

        if (getPageName() === 'longterm') {
            // Page Historique : pilotée par la sélection de période.
            initLongtermControls();
        } else {
            // Tableau de bord : fenêtre glissante de 2 h.
            fetchHistory();
            setInterval(fetchHistory, HISTORY_REFRESH_MS);
        }
    }

    if (isStatsPage()) {
        fetchStats();
        setInterval(fetchStats, STATS_REFRESH_MS);
    }

    setInterval(fetchLive, LIVE_REFRESH_MS);
    setInterval(fetchAlert, ALERT_REFRESH_MS);
};
