# [1.38.0] - 2026-09-16

### Added

- **Probe diagnostics in the ESP-NOW frame (protocol v2).** Each frame now carries
  the probe's `reset_reason` (esp_reset_reason: POWERON/BROWNOUT/DEEPSLEEP/PANIC…)
  and a RTC `wake_count` (survives deep sleep, resets on a power cycle). On every
  received OUT frame the hub logs `[OUT] frame recue seq=… wake=… reset=… …`, so
  when a frame returns after a gap we can read WHY the outdoor probe dropped out
  (a BROWNOUT/POWERON points to power, a wake_count jump to failed wake cycles)
  without ever plugging it in - it is deployed outdoors. Frame grows 51→54 bytes;
  the protocol version bump means the probe and the hub must be reflashed together.

# [1.37.0] - 2026-09-16

### Added

- **Forecast fetch bounded and logged.** The forecast HTTP GET runs in `loop()`
  and had no explicit timeout (default ~5 s, longer if the server hangs). It now
  sets `setConnectTimeout(3000)` / `setTimeout(4000)` so it can never stall the
  loop for long (and delay draining the ESP-NOW queue). It also logs
  `[FORECAST] start` and `[FORECAST] done in Nms (code X)`, and every received OUT
  frame logs `[OUT] frame recue t=… batt=…`. Captured by morfMonitor, these
  timestamps let us correlate any OUT-frame gap with a fetch window (testing the
  "fetch vs ESP-NOW reception collision" hypothesis) instead of guessing.

# [1.36.0] - 2026-09-16

### Added

- **Memory auto-recovery guard.** The web UI (ESPAsyncWebServer/AsyncTCP) could
  stop responding after several hours without the device rebooting, requiring a
  manual power-cycle: long SD reads keep feeding the task watchdog, so it never
  fires. A `heapGuard()` in the main loop now watches both the free heap and the
  largest allocatable block (AsyncTCP needs contiguous memory) and restarts
  cleanly when either stays critical for a grace period. Thresholds are in
  `config.h` (`HEAP_MIN_FREE_BYTES`, `HEAP_MIN_BLOCK_BYTES`, `HEAP_LOW_GRACE_MS`).
- **Health metrics in `/status`.** The endpoint now carries a `metrics` block with
  `free_heap_b` and `free_block_b`, so morfMonitor can record a 48 h FIFO of heap
  and uptime and diagnose a future freeze (declining heap, moment of the break).

# [1.35.0] - 2026-09-15

### Added

- **Dedicated "Capteur" OLED page** for the outdoor probe, inserted between System
  and Logs: battery (voltage + %, or "Bat: absente"), ESP-NOW channel, frame
  counters (received `rx` / CRC-valid `ok`) and freshness of the last frame. The
  System page is back to its original four lines (Heap/PSRAM/Flash/Ver): the probe
  battery line added in 1.34.0 overflowed the 64 px screen, so it moved to its own
  page. User guide now lists every OLED page with ASCII mockups.

# [1.34.1] - 2026-09-15

### Fixed

- **OLED history graphs were empty.** Two single-sensor leftovers: the graph read
  RAM only (`getRecentHistory`/`getOutdoorHistory`), nearly empty after an erase or
  reboot while the full history is on SD; and the line was cut whenever two points
  were more than 90 s apart, a threshold from the old 1/min cadence that, at the
  current 5 min cadence, skipped *every* segment (nothing drawn). The OLED graph now
  reads SD+RAM over a 24 h window (same source as the web, cached 30 s to avoid
  hammering the card at the 1 Hz refresh), keeps only valid slices (no holes), and
  connects them with a `max(2.5 slices, 20 min)` threshold like the web and
  morfAnalytics. Time axis relabelled -24h.
- **Browser served the old UI after a reflash.** The embedded pages and `app.js`
  carried no cache headers, so browsers reused a stale `app.js` (empty/frozen
  charts until a manual hard-refresh). Every response now sends
  `Cache-Control: no-cache`, forcing revalidation.

### Removed

- **Dead single-sensor history code** in `app.js` (`fetchHistory`, `updateChart`,
  `buildHistoryUrl`, `getHistoryWindowSeconds`, `getHistoryIntervalSeconds`,
  `HISTORY_WINDOWS_SECONDS`, `HISTORY_REFRESH_MS`): a leftover path that hit the
  `ctx`-less window-mode endpoint. The live History page uses the ctx-aware
  `refreshLongterm`; these were never called.

# [1.34.0] - 2026-09-15

### Fixed

- **History graphs were empty (IN) or nearly empty (OUT).** The History page reads
  through `queryRange`, which gated SD access on `isAvailable()`. That call probes
  `SD.cardType()` and, on a transient `CARD_NONE` glitch (common under web load),
  tore down the mount and entered a reconnect cooldown, so the query silently fell
  back to RAM only: indoor history looked empty and outdoor showed just the few
  recent frames, while the data was safely on the card (the raw export used by
  morfAnalytics reads via `ensureMounted()` and stayed full). All history read
  paths (`queryRange`, `querySynthesis`, the sample-near lookups, CSV export) now
  use `ensureMounted()` like the raw/days endpoints; write and card-removal paths
  keep `isAvailable()`.

### Added

- **Remote sensor battery voltage shown, not just the percentage.** The dashboard
  Outdoor card now reads `3.98 V (72 %)` and the OLED System page carries a
  `Sonde: 3.98V 72%` line, visible at all times (previously the battery only
  appeared on the OLED as a low-battery alert). Data already came in the ESP-NOW
  frame and `/api/live`.

### Changed

- **Clear-history control is now harder to reach.** The System page "Historique"
  card is collapsed by default (a `<details>`): it must be expanded before the
  destructive "Vider tout l'historique" button appears, so a distracted click
  cannot wipe the history. The existing double confirmation is unchanged.

# [1.33.3] - 2026-09-14

### Fixed

- **History OUT chart no longer combs.** On a single-source view (Outdoor or
  Indoor) the empty slices are no longer filled with the other source: injecting
  an indoor point into an outdoor chart connected two unrelated values and drew a
  misleading saw-tooth (worse on short custom ranges, where the slice is finer
  than the 5 min outdoor cadence so most slices were filled). The chart now draws
  only the requested source and connects points across empty slices, breaking the
  line only on a real sensor silence (gap > max(2.5 slices, 20 min), the same rule
  as morfAnalytics). The "N slices filled by indoor" note is gone.

# [1.33.2] - 2026-09-14

### Documentation

- **Docs realigned with the current firmware.** README (en+fr) and the docs
  guides now describe the symmetric history layout (`/history/indoor/…` and
  `/history/outdoor/…`) instead of the old flat `/history/YYYY/MM/…`, the
  view-only History page (Source Outdoor/Indoor/both + Period, comparison
  removed), the remote wipe (`POST /api/history/clear`), the 5 min outdoor
  cadence and the forecast archive. No behaviour change.

# [1.33.1] - 2026-09-14

### Fixed

- **Outdoor card at boot.** Right after a reboot the card showed the disk-seeded
  (old) value with a fantasy "last frame" date. `awaiting_first` now takes
  priority: the OUT values show "--" and the card/hero say "en attente du premier
  relevé après redémarrage (max N min)" until the first real frame arrives, whatever
  the stale seed contains.
- **History chart not applying a custom range.** In IN+OUT the two history requests
  were fired concurrently; the single-threaded ESP32 could fail one (SD streaming),
  leaving the chart stuck on the previous 24 h. Requests are now sequential and the
  target point count trimmed for margin, so a selected range (including a custom
  one) actually loads and the time axis follows it.
- **Removed em dashes** from the web UI (dashboard, stats, system, history), per
  house style: hyphens, colons or middots instead.

# [1.33.0] - 2026-09-14

### Changed

- **History page simplified: view, not analysis.** The page is now just Source +
  Period + chart. The period-vs-period comparison ("Même période / Autre période")
  is removed - deep analysis lives in morfAnalytics now, and MeteoHub should stay a
  station that shows what was measured.
- **New "Intérieur + Extérieur" source.** IN+OUT draws both series on one time axis
  (outdoor solid, indoor dashed, same units/axes), so the gap, damping and lag are
  visible at a glance. In IN+OUT, the Synthèse panel shows the outdoor stats with
  the mean OUT − IN gap per metric. The OUT view keeps its indoor gap-filling
  (orange-marked), and the time axis keeps adapting to the selected period.

# [1.32.0] - 2026-09-14

### Added

- **Forecast archiving for the forecast-vs-observed analysis (weather chantier
  step 9, device side).** MeteoHub now archives a daily "day-ahead" forecast
  snapshot: periodically (~30 min, when NTP is set and the forecast is plausible)
  it stores tomorrow's forecast under its target day, rewriting so the file for
  day D converges to the last D-1 forecast for D. Flat JSON files
  `/history/forecast/AAAA-MM-JJ.json` (low volume, ~1/day). New route
  `GET /api/forecast/history?from=&to=` streams the archived snapshots for
  morfAnalytics to collect. Cleared by the existing full history wipe.

# [1.31.0] - 2026-09-14

### Changed

- **Outdoor card wording right after a reboot.** Before the first frame is
  received, the card said "sonde extérieure absente" (which reads as a failure),
  when the truth is the hub is simply waiting for the sensor's first transmission.
  `/api/live` now reports `out.awaiting_first` (no frame received since boot) and
  `out.interval_s`; the card then shows "en attente du premier relevé après
  redémarrage (max N min)" instead of the absence warning. Genuine loss after the
  sensor had been seen still shows "sonde extérieure absente". (The OLED already
  distinguished this case with its radio-debug line.)

# [1.30.0] - 2026-09-14

### Removed

- **Home page history graph.** It showed indoor (IN) data, which is not what a
  weather home should highlight. History lives on the History page, with period
  selection. Chart.js is no longer loaded on the home page (faster load).

### Changed

- **History chart X axis follows the selected period.** Tick labels now adapt to
  the span (HH:MM for up to 24 h, DD/MM HH:MM up to a week, DD/MM beyond) with a
  clean tick density, instead of a fixed format. The category axis is kept so the
  period-vs-period comparison (aligned by index) still works.
- **History chart loads faster.** The period-A, comparison-B and indoor-fill
  requests are now fetched in parallel instead of sequentially.

# [1.29.0] - 2026-09-14

### Changed

- **History page auto-refresh now follows the recording cadence.** It refreshed
  every 15 s even though a new sample is only recorded every ~5 min. `/api/history`
  now reports `measurement_interval_s` (single source: `INDOOR_MEASUREMENT_INTERVAL_SECONDS`)
  and the page schedules its auto-refresh at that interval plus a small margin.
  Manual "Actualiser" is unchanged.

### Fixed

- **History chart no longer looks empty with a single measurement.** An isolated
  valid point (both neighbours null, e.g. the first sample right after a reset)
  now shows a marker; a line alone cannot draw a single point, so the chart looked
  empty while Statistics showed the value.

# [1.28.1] - 2026-09-14

### Changed

- **Outdoor history is written only from a real, valid received frame.**
  Hardened `addOutdoor()` to reject an invalid frame outright (no live update, no
  archive) instead of relying on the caller to filter. Confirms and locks the
  invariant: an OUT record only ever comes from a physically received ESP-NOW
  frame, never from the indoor fallback, the "effective" display value, or a
  value seeded from disk at boot. After a reboot, OUT stays "awaiting first
  reception" (no OUT history) until the sensor's first frame; indoor archival
  continues normally in the meantime. New accessor
  `hasReceivedOutdoorSinceBoot()` exposes that state.

# [1.28.0] - 2026-09-14

### Added

- **Clear history from the web UI.** New `POST /api/history/clear` route (calls
  `clearHistory()`) and a "Vider tout l'historique" button in the System page,
  behind a double confirmation. Erasing the history no longer requires the on-device
  OLED menu.

# [1.27.0] - 2026-09-14

### Changed

- **Symmetric IN/OUT history layout.** Indoor storage was flat under `/history`
  while outdoor lived under `/history/outdoor`; the two trees are now identical:
  - SPIFFS: `/history/indoor_recent.dat` and `/history/outdoor_recent.dat`.
  - SD: `/history/indoor/AAAA/MM/AAAA-MM-JJ.bin (+.stats)` and
    `/history/outdoor/AAAA/MM/...`.
  All indoor paths (recent file, daily bin/stats, day listing, raw export for the
  morfAnalytics collector) moved from `/history/...` to `/history/indoor/...`.

### Fixed

- **`clearHistory()` now wipes everything** for a clean restart: both RAM series
  (IN + OUT), both SPIFFS recent files (plus the pre-symmetry `recent.dat`), and
  the whole SD `/history` tree, then recreates the empty symmetric structure.
  Previously it left the outdoor recent file and OUT RAM state behind.

# [1.26.0] - 2026-09-14

### Added

- Outdoor card: the exact time of the last reading is now shown next to the
  freshness label, as `jj/mm - hh:mm:ss` (e.g. "à jour (14/09 - 15:23:41)"). The
  hub has no reliable clock, so the browser reconstructs it from the frame age
  (`age_ms`) reported by `/api/live`: now minus age. Shown for both fresh and
  stale outdoor readings.

# [1.25.2] - 2026-09-14

### Changed

- Indoor (IN) display is live again: `/api/live` and the OLED weather page read
  the local indoor sensor in real time on each refresh (the sensor is local and
  cheap to read), while archival keeps the 5-min cadence. Only the outdoor value
  stays "last frame received" (it arrives by radio). Reverts the 1.25.0 cached-IN
  behaviour; a live read never creates a history entry.

# [1.25.1] - 2026-09-14

### Fixed

- Outdoor freshness thresholds realigned to the 5-min sensor cadence:
  `OUTDOOR_FRESH_MAX_MS` 90 s → 7 min and `OUTDOOR_UNAVAILABLE_MS` 10 → 20 min.
  With the new cadence the old 90 s window marked OUT as stale between every
  frame (the sensor now sends every ~5 min).

# [1.25.0] - 2026-09-14

### Changed

- Indoor (IN) measurement cadence is now configurable via
  `INDOOR_MEASUREMENT_INTERVAL_SECONDS` (config.h), **default 300 s (5 min)**,
  aligned with the outdoor sensor so the IN and OUT history series are
  homogeneous for morfAnalytics (was a hard-coded 60 s).
- The measurement cadence and the UI refresh are now cleanly separated. The web
  dashboard (`/api/live`) and the OLED weather page return the **last known
  measurement** (`SensorManager::last()`) instead of triggering a fresh
  acquisition on every refresh: the UI stays quasi-instant without creating a new
  measurement or history entry. A reading at boot primes the cache.

# [1.24.1] - 2026-09-13

### Fixed

- Statistics page "24 h" summaries actually cover 24 h now. `getIndoorStats` /
  `getOutdoorStats` aggregated the whole RAM history instead of a 24 h window: the
  indoor cache, loaded uncapped from LittleFS at boot, could hold weeks of data
  (tens of thousands of records), inflating the indoor ranges so they looked like
  outdoor weather. Stats are now bounded to the last 24 h, and `loadRecent` keeps
  only the last `MAX_RECENT_RECORDS` in RAM (the long archive lives on SD).

# [1.24.0] - 2026-09-13

### Added

- Low-battery alert for the remote (outdoor) sensor. The battery level carried in
  the ESP-NOW packet (dropped until now) reaches OutdoorData, `/api/live` (out
  block: `battery_pct`/`battery_v`/`battery_low`), the web dashboard (battery row
  + a warning under the OUT card) and the OLED weather page (a "PILE SONDE xx% !"
  line that takes priority). Threshold `OUTDOOR_BATTERY_LOW_PCT` (20 %).
- Outdoor barometric trend engine: `getTrendOutdoor()` (shared root-parameterized
  core), reading the OUT RAM history and OUT SD samples, with no cross-context
  fallback (an OUT trend never borrows an IN point).

### Fixed

- Statistics page: the weather trend was computed from indoor data while labelled
  as weather. It now uses the outdoor stream (`/api/stats?ctx=out` returns the OUT
  trend); the indoor and outdoor summaries stay clearly separated.

# [1.23.0] - 2026-09-13

### Added

- History page: in the OUT view, time slices with no outdoor measurement are
  filled from the indoor series and marked (orange dots on the curve, with a
  "N slices filled from indoor" note). The fallback stays explicit — an indoor
  point is never shown as a real outdoor measurement.
- Statistics page: each summary now shows its sample count, so outdoor vs indoor
  coverage is visible at a glance.

# [1.22.0] - 2026-09-13

### Added

- Outdoor history is now fully served: real `queryOutdoorRange` and
  `exportOutdoorCsv` (shared root-parameterized core), plus `getOutdoorStats`.
  `/api/history`, `/api/stats` and `/api/history/export.csv` accept `ctx=out|in`
  (default `in`, legacy-compatible; responses echo `ctx`).
- Statistics page shows indoor AND outdoor summaries side by side (trend stays
  indoor-only for now, no outdoor trend engine yet).
- History page gains an OUT/IN source selector (defaults to OUT). Period and
  comparison filters apply to whichever source is selected.

# [1.21.0] - 2026-09-13

### Changed

- Web dashboard redesigned as a weather station. A hero shows the effective
  outdoor temperature/humidity (OUT when available, otherwise a clearly labelled
  indoor fallback), an OUT card (temperature, humidity, pressure + freshness) and
  an IN comfort card, consuming the `in`/`out`/`effective` blocks of `/api/live`.
  Indoor fallback and stale outdoor data are always shown with their provenance,
  never presented as a real outdoor measurement. The 2 h chart is labelled as
  indoor history.

# [1.20.0] - 2026-09-13

### Added

- OLED now has the full set of six history graph pages: IN and OUT each for
  temperature, humidity and pressure (was only IN temp, IN hum, OUT pres).

### Changed

- OLED weather page marks indoor fallback discreetly: when OUT is unavailable
  and the indoor sensor is valid, the OUT row shows the indoor value prefixed
  with an "I" (e.g. `I18.5C`), so a fallback reading is never mistaken for a
  real outdoor measurement. Pressure stays outdoor-only on screen (the indoor
  pressure fallback remains available in the data layer, /api/live effective).

# [1.19.0] - 2026-09-13

### Changed

- OLED weather page is now freshness-aware. OUT shows its last value only while
  a recent radio frame exists: fresh shows the weather description, stale shows
  `OUT~` plus the age (`OUT perime ~Nmin`), and once unavailable OUT reads `--`
  with `OUT absent -> IN` (the indoor line above is then the current reference).
  A `live` value seeded from disk at boot counts as unavailable, so old data is
  never shown as current.

# [1.18.0] - 2026-09-13

### Added

- `GET /api/live` now exposes provenance-aware readings: an `in` block (local
  sensors), an `out` block (last ESP-NOW frame, with `age_ms` and `fresh`), and
  an `effective` block that resolves each metric to the value to display while
  keeping its source (OUT fresh / OUT stale / IN fallback / unavailable) via the
  meteo_context resolver. The legacy flat `temp`/`hum`/`pres` fields are kept, so
  the current dashboard is unaffected.

# [1.17.0] - 2026-09-13

### Added

- Collection routes are now context-aware: `/api/history/days?ctx=out|in` and
  `/api/history/raw?ctx=out|in` serve the outdoor or indoor stream. The default
  stays `in` (legacy), so an existing collector that omits `ctx` is unaffected.
  Responses echo `"ctx"`. Outdoor days/records read from `/history/outdoor`.
- `HistoryManager::listDaysOutdoor()` / `exportRawOutdoor()` (shared core
  parameterized by storage root), so morfAnalytics can collect the OUT stream
  (the real weather) in addition to IN.

# [1.16.0] - 2026-09-13

### Added

- Effective-reading model (`meteo_context.h`): resolves, per metric, the value
  to display while keeping its provenance (OUT fresh / OUT stale / IN fallback /
  unavailable). Groundwork so the OLED and web can show OUT with an explicit IN
  fallback, without ever faking an outdoor measurement.
- Outdoor freshness tracking: `HistoryManager::outdoorAgeMs()` from the last
  received radio frame, with thresholds `OUTDOOR_FRESH_MAX_MS` and
  `OUTDOOR_UNAVAILABLE_MS` in `config.h`.

### Changed

- Indoor archival now goes through an explicit `addIndoor(IndoorData)` entry
  point (the legacy history is the IN stream: decision "legacy = IN").

### Removed

- Dead parallel "indoor" storage helpers (`_indoorHistory`, and the
  save/ensure/build/update/read `*Indoor*` day methods) made redundant by the
  legacy = IN decision. The outdoor storage path is unchanged.

# [1.15.6] - 2026-09-13

### Changed

- ESP-NOW RX cleaned: STA broadcast peer, no 1 Mbps / open-AP / join hacks.
- SoftAP `MH-NOW` is a WPA2 channel beacon; the probe does not associate.

# [1.15.5] - 2026-09-13

### Fixed

- `MH-NOW` is an open SoftAP (C3 WPA2 AUTH_EXPIRE on S3 AP).
- ESP-NOW listen on STA interface at 1 Mbps so an unassociated C3 is heard.

# [1.15.4] - 2026-09-13

### Fixed

- SoftAP `MH-NOW` forced to WPA2-PSK after `softAP()` so C3 STA can complete
  the 4-way handshake (was AUTH_EXPIRE).

# [1.15.3] - 2026-09-13

### Changed

- ESP-NOW status log includes last source MAC and SoftAP station count so a
  C3 probe that never reaches `recv_cb` is obvious (`rx=0`).

# [Non publié]

# [1.15.1] - 2026-09-12

### Changed

- Reverted to classic ESP-NOW mode with AP-based communication.
- Sensor connects to MH-NOW AP for reliable ESP-NOW broadcast.
- Receiver uses AP interface for broadcast reception.

# [1.15.0] - 2026-09-12

### Changed

- Simplified ESP-NOW receiver to use unicast direct mode instead of broadcast.
- Changed peer registration to use specific MeteoHubSensor MAC (F0:F5:BD:FB:5E:88) on STA interface.
- Added OLED display support for outdoor data (pressure from OUT if available).
- Improved ESP-NOW interface selection for better communication reliability.

# [1.14.9] - 2026-09-12

### Fixed

- SoftAP `MH-NOW` now starts at boot on channel 6 (do not wait for Livebox).
  Recreate the AP once after STA join so the handshake does not kill it.
  Probe `apsta=0` meant the AP was missing, not that ESP-NOW was "fine".

# [1.14.8] - 2026-09-12

### Changed

- Hub AP `MH-NOW` is visible (WPA2) on the STA channel so the C3 probe can
  associate. Status log includes `apsta=` (connected probes). Flash the hub
  before the probe.

# [1.14.7] - 2026-09-12

### Fixed

- ESP-NOW RX stayed at 0 while the probe sent on ch 6. Hub now uses AP+STA
  with a hidden AP on the STA channel so ESP-NOW is received on the AP
  interface. OLED OUT stays `--` until `rx` increments.

# [1.14.6] - 2026-09-12

### Added

- ESP-NOW status log includes HT40 secondary (`sec=none|above|below`) next to
  the primary channel.

# [1.14.5] - 2026-09-12

### Fixed

- After association, refuse a 5 GHz STA channel (same Orange SSID, band
  steering): the C3 probe only transmits on 2.4 GHz. The Net. page shows
  `5GHz!` when that happens. ESP-NOW recv no longer logs from the Wi-Fi task.

# [1.14.4] - 2026-09-12

### Added

- Log every raw ESP-NOW RX length so a silent OLED can be told apart from CRC
  rejects (`NOW chX rxY okZ` on the weather page).

# [1.14.3] - 2026-09-12

### Fixed

- STA is forced to 2.4 GHz (11b/g/n) so the S3 does not join the same SSID on
  5 GHz, which made ESP-NOW from the C3 probe invisible.

# [1.14.2] - 2026-09-12

### Added

- OLED **Net.** page shows the STA Wi-Fi channel and MAC address, so the outdoor
  probe can be checked against the radio the hub actually listens on.

# [1.14.1] - 2026-09-12

### Fixed

- **OLED OUT stayed at `--` even when the probe showed `NOW: OK`.** Three causes.
  (1) STA modem sleep: the S3 radio sleeps between AP beacons and misses
  ESP-NOW; broadcast TX still reports success on the probe. Modem sleep is now
  forced off (`WiFi.setSleep(false)` / `WIFI_PS_NONE`). (2) Broadcast peer was
  not registered on `WIFI_IF_STA`. (3) ESP-NOW is re-initialized after STA
  association so it follows the AP channel. The weather page shows `NOW chX rxY
  okZ` until a valid OUT frame arrives.

# [1.14.0] - 2026-09-12

### Added

- **Outdoor ESP-NOW ingest on the station.** Valid `MeteoPacket` frames from the
  remote probe are queued in the Wi-Fi task and decoded in `loop()`, then stored
  as OUT history. Live OUT values stay available for the OLED even if NTP is
  not synced yet (archive waits for a reliable clock).

- **OLED weather page: IN + OUT columns.** Indoor T/H stay on the first line.
  Outdoor T/H share the same temperature and humidity columns. Atmospheric
  pressure is the OUT probe value only (never the indoor BMP280). The pressure
  graph page follows the same rule.

- `docs/interface_web.md`: illustrated overview of the web pages served by
  MeteoHub (dashboard, statistics, system), with anonymized sample captures.
  Added to the documentation index.

### Fixed

- **Web OTA: `Update.begin()` failed and blocked further updates (USB flash
  required).** Two causes. (1) The partition table was not pinned: the layout
  followed the board default, and without two app slots `ota_0`/`ota_1` OTA is
  impossible ("OTA begin failed"). It is now fixed in `partitions.csv`
  (`board_build.partitions`), dual-OTA 6.25 MB, matching what is already in
  flash. (2) A failed OTA left the `Update` object "in progress", so the next
  attempt failed until reboot; the handler now aborts a leftover session
  (`Update.abort()`) before opening a new one, and on write failure. Changing
  the partition table requires one USB flash; OTA then works again.

# [1.13.3] - 2026-08-20


### Corrigé

- La version est montée après les évolutions de packaging afin que le tag source
  identifie exactement le commit produisant le firmware publié.

# [1.13.2] - 2026-07-31

### Modifié

- Le lien **Analyses avancées** ouvre désormais directement l'espace météo de
  morfAnalytics (`/meteohub`) plutôt que le portail général.

# [1.13.1] - 2026-07-26

### Ajouté

- **Déclaration de l'API dans `/status`.** MeteoHub annonce désormais, en plus
  de son interface web, la liste de ses routes de données (`GET /api/live`,
  `/api/history`, `/api/history/summary`, `/api/history/export.csv`,
  `/api/stats`, `/api/analytics`, `/api/alert`, `/api/system`) sous la clé `api`,
  au même format que les services Linux du parc. morfMonitor les cartographie et
  un collecteur peut les suivre sans connaître MeteoHub à l'avance.

  **Rien ne change pour l'appareil seul.** L'API n'est pas une capacité du
  heartbeat : elle ne figure que dans le document `/status`, servi uniquement
  quand on l'interroge. Sans récepteur sur le réseau, MeteoHub mesure, stocke,
  trace et exporte exactement comme avant - il ne diffuse rien de plus. La liste
  est en flash (PROGMEM), sans coût mémoire notable. Nécessite morfBeacon 0.5.1
  (émetteur Arduino, re-vendoré).

# [1.13.0] - 2026-07-21

### Ajouté

- **Annonce de présence sur le LAN (protocole `morfbeacon/1`).** MeteoHub
  *écoutait* déjà ce protocole pour repérer un service d'analyse ; il l'**émet**
  désormais, et devient donc découvrable par le même mécanisme que les services
  Linux et Windows du parc.

  Auparavant il n'était trouvé que par sonde TCP, ce qui suppose de connaître son
  nom mDNS à l'avance - l'inverse d'une découverte, et la raison pour laquelle
  des listes statiques restaient nécessaires dans `morfsystem.json`.

- **Route `GET /status`** au format `morfbeacon/1`, et déclaration de la capacité
  `web_ui` : un observateur du parc peut proposer un lien vers l'interface de
  MeteoHub **sans rien connaître de MeteoHub**. La capacité annonce, `/status`
  détaille - déclarer l'une sans servir l'autre désignerait une interface que
  personne ne saurait ouvrir.

  Émetteur vendoré dans `third_party/morf/beacon-arduino/`, copie de
  `arduino/morfbeacon_emitter.h` du dépôt morfBeacon. Le partage porte sur le
  **protocole**, pas sur le code : la bibliothèque Qt ne tourne pas sur un ESP32.

- **Fichier `VERSION` à la racine, désormais autoritaire.** La version était
  écrite en dur dans `platformio.ini` (`-D PROJECT_VERSION='"1.12.0"'`), donc à
  un endroit différent des onze autres projets de l'écosystème, où un fichier
  `VERSION` fait autorité. Aucun outil ne pouvait établir l'inventaire des
  versions du parc, puisque deux projets sur quatorze publiaient la leur
  autrement.

  `scripts/version.py` lit ce fichier et injecte `PROJECT_VERSION` à la
  compilation. Ajouter le fichier **sans** ce script aurait créé deux sources de
  vérité pour une même valeur - le défaut que le script existe précisément pour
  éviter. `platformio.ini` ne porte plus la version, il la lit.

- **`LICENSE`** (GPL-3.0-only), **`CONTRIBUTING.md`** et **`ROADMAP.md`**, qui
  manquaient alors que le reste de l'écosystème les fournit. `CONTRIBUTING.md`
  consigne les contraintes propres à la cible ESP32-S3 - mémoire comptée,
  corruption de carte SD, dérive d'horloge, chutes du bus I2C - qui expliquent
  des choix du code que rien ne justifiait par écrit.

### Corrigé

- **Le brochage SPI de la carte SD documenté ne correspondait pas au firmware.**
  `hardware_wiring.md`, `pin_mapping.md` et `maintenance_and_troubleshooting.md`
  annonçaient CLK 21 / MISO 47 / MOSI 38 / CS 39, alors que `board_config.h`
  utilise **13 / 12 / 11 / 10** depuis sa révision du 02/07/2026. Un lecteur qui
  câblait d'après la documentation obtenait une carte SD non détectée, et le
  guide de dépannage lui faisait vérifier les mêmes broches erronées. Les trois
  documents sont alignés sur `board_config.h`, seule source de vérité.
- `docs/pin_mappnig.md` renommé en `docs/pin_mapping.md` (coquille dans le nom).
  Ce document de 119 lignes n'était référencé nulle part et absent de
  `docs/index.md` : le brochage réel de la carte était invisible pour le lecteur.
  Il est maintenant listé dans l'index.
- `docs/beginner/faq.md` : chemin des sources corrigé (`src/modules/sensors.*`).
- `docs/beginner/readme.md`, page d'accueil du dossier sur GitHub, dupliquait
  `index.md` sans aucun lien : ses entrées sont désormais cliquables.

# [1.12.0] - 2026-07-19
### Changed
- **Le service d'analyse est reconnu à sa CAPACITÉ, plus à son nom.** MeteoHub cherchait un heartbeat dont le champ `app` valait exactement `morfAnalytics`. Or le projet est sous licence GPL : chacun peut renommer son service, et la détection cessait alors de fonctionner. MeteoHub cherche désormais un service annonçant la capacité **`advanced_analysis`** (nouveau champ `capabilities` du protocole morfBeacon, voir morfBeacon 0.2.0) et n'utilise le nom annoncé que comme **libellé** affiché dans le menu et la page Système. Renommer son service n'interrompt plus l'intégration.

  **Conséquence de mise à jour** : un service d'analyse antérieur à morfAnalytics 0.4.0 n'annonce pas de capacité et n'est donc plus détecté. Le mettre à jour, ou renseigner son adresse manuellement (ci-dessous).
- **L'entrée de menu s'ouvre dans le même onglet** (`data/menu.js`), au lieu d'un nouvel onglet. L'utilisateur passe d'un service à l'autre comme entre deux pages, sans accumuler d'onglets ; le service d'analyse propose en retour un lien « Retour à MeteoHub ». Chaque application conserve par ailleurs son indépendance technique et son propre cycle de vie.
- **L'entrée de menu porte le nom annoncé par le service**, et non un libellé figé.

### Added
- **Adresse manuelle du service d'analyse** (page **Système** → Analyse avancée → « Adresse manuelle »). La découverte automatique suppose que les annonces UDP atteignent la station, ce qui n'est pas le cas sur un réseau segmenté, à travers un VPN, ou avec un point d'accès isolant les clients. Une adresse peut alors être saisie ; elle est **persistée en NVS**, survit au redémarrage et **prend le pas** sur la découverte. Vider le champ rétablit le mode automatique. Une adresse ne commençant pas par `http://` ou `https://` est refusée avec un message explicite.
- **API enrichie** : `GET /api/analytics` renvoie désormais `{available, mode, effective_url, capability, manual_url, detected:{found,name,version,host,status_port,last_seen_s}}` ; `POST /api/analytics/config` (paramètre `manual_url`) enregistre ou efface l'adresse manuelle.
- **Guide débutant [docs/analyse_avancee.md](docs/analyse_avancee.md)** : à quoi sert un service d'analyse, comment la détection fonctionne, comment naviguer entre les deux applications et que faire si la découverte automatique ne passe pas.

### Notes
- La sélection explicite entre plusieurs services détectés et la personnalisation du nom affiché sont volontairement reportées : elles répondent à des besoins plus spécifiques et n'apportent rien au fonctionnement courant. En présence de plusieurs services, MeteoHub retient le dernier annoncé ; une adresse manuelle permet d'en imposer un.
- Les emplacements de captures d'écran de `docs/analyse_avancee.md` sont **en attente des images** (voir `docs/images/README.md`, qui décrit précisément ce qu'il faut cadrer).

# [1.11.2] - 2026-07-19
### Fixed
- **`/api/history/summary` et `/api/history/export.csv` renvoyaient le JSON de l'historique** (bug préexistant, antérieur à la 1.11.0). ESPAsyncWebServer fait correspondre une route à toute URL qui **commence** par elle : `/api/history`, enregistrée avant ses sous-routes, les captait toutes. La synthèse renvoyait donc la liste des mesures, et le bouton **Export CSV** téléchargeait un fichier JSON portant l'extension `.csv`. La route générique est désormais déclarée **en dernier**, après toutes ses sous-routes (`src/managers/web_manager.cpp`).
- **`/api/history/days` et `/api/history/raw` ne répondaient jamais**, pour la même raison : captées par `/api/history`, elles tombaient dans une branche qui n'émet aucune réponse pour leurs paramètres.
- **Journées fantômes dans `/api/history/days`.** Les fichiers `.stats` étaient pris pour des fichiers de mesures : `sscanf` renvoie le nombre de conversions **affectées**, si bien que `"%4u-%2u-%2u.bin"` acceptait `AAAA-MM-JJ.stats`, l'échec du littéral `.bin` final n'étant pas compté. Chaque journée apparaissait deux fois, la seconde avec des horodatages aberrants (2005-2014). L'extension est désormais vérifiée séparément de la date.
- **`/api/history/raw` échouait au-delà d'environ 300 enregistrements** (réponse abandonnée, aucun octet renvoyé). Deux causes : la boucle d'émission ne rendait jamais la main à la tâche réseau (ajout de `COOPERATIVE_YIELD_EVERY`, comme le fait déjà `/api/history`), et la borne haute était fixée à 2880, très au-delà de ce que le flux de réponse asynchrone tient réellement. Elle est ramenée à **250**, mesurée sûre sur l'appareil ; le collecteur pagine.

  Ces quatre défauts ne sont observables que sur le matériel : ils compilent sans le moindre avertissement.

# [1.11.1] - 2026-07-19
### Fixed
- **`GET /api/history/days` ne répondait jamais.** La route restait bloquée indéfiniment (aucune réponse, l'appareil restant par ailleurs parfaitement fonctionnel) : `HistoryManager::listDays()` **imbriquait** les parcours de répertoires, gardant ouverts en même temps les itérateurs de `/history`, `/history/AAAA` et `/history/AAAA/MM`. Maintenir plusieurs itérations de répertoires simultanées bloque la lecture de la carte SD sur ESP32. Chaque niveau est désormais **entièrement lu et refermé avant de descendre** au suivant (`listEntries()`), avec un garde-fou sur le nombre d'entrées. Les horodatages extrêmes de chaque journée sont en outre lus dans l'en-tête du fichier plutôt que par deux positionnements supplémentaires, chaque accès SD étant coûteux.

  Le défaut n'était pas détectable à la compilation : il ne se manifeste que sur une vraie carte SD.

# [1.11.0] - 2026-07-19
### Added
- **API de recopie de l'historique pour un serveur d'analyse (lecture seule).** Deux nouvelles routes permettent à **morfAnalytics** de constituer sa copie de travail sans jamais rien modifier sur MeteoHub, qui demeure la **source de vérité** :
  - `GET /api/history/days` - journées présentes sur la carte SD, avec pour chacune le nombre de mesures enregistrées (`{day, nrec, first_ts, last_ts}`) ;
  - `GET /api/history/raw?day=AAAAMMJJ&index=N&limit=M` - mesures brutes de la journée à partir de la position `N`, au format compact `[horodatage, température, humidité, pression]` (~30 octets par mesure au lieu de ~55 en objet nommé).

  Une mesure est repérée par sa **position dans le fichier du jour**, et non par son horodatage : les fichiers `.bin` étant écrits en **ajout seul**, cette position ne change jamais, alors qu'un horodatage peut **reculer** lors d'un recalage NTP ou **se répéter** lors du passage à l'heure d'hiver - un repère temporel ferait donc sauter ou dupliquer des mesures. Le serveur d'analyse retient le couple (jour, position) et ne redemande que ce qui lui manque. Implémentation `HistoryManager::listDays()` / `exportRaw()` (`src/managers/history_manager.*`), réutilisant le parcours séquentiel par blocs introduit en 1.8.0.
- **Entrée de menu « Analyse avancée ».** Lorsque le service morfAnalytics est détecté sur le réseau local (`GET /api/analytics`), un lien vers sa page d'analyse est inséré au menu **juste avant « Système »**, qui reste la dernière entrée (`data/menu.js`). En l'absence de service détecté, le menu est strictement inchangé : **aucune dépendance** n'est introduite.

### Notes
- Ces deux routes ne servent qu'à un serveur d'analyse. Pour un export manuel, le **CSV** (`GET /api/history/export.csv`) reste la voie recommandée, directement exploitable dans un tableur. (Cet export était en réalité cassé jusqu'à la 1.11.2 - voir cette version.)

# [1.10.0] - 2026-07-19
### Added
- **Détection optionnelle du service morfAnalytics (écosystème morfSystem).** MeteoHub écoute passivement le heartbeat morfBeacon (`morfbeacon/1`, broadcast UDP sur le port `45454`) et signale la présence du moteur d'analyse `morfAnalytics` (`src/modules/analytics_beacon.*`, API `GET /api/analytics`). La page **Système** affiche « Analyse avancée disponible / indisponible ». **Aucune dépendance** : si aucun serveur n'est détecté, le comportement nominal de MeteoHub (mesures, historique, graphiques, exports) est strictement inchangé. MeteoHub reste la **source de vérité** ; ce module ne fait que constater une présence. Configurable dans `include/config.h` (`ANALYTICS_BEACON_ENABLED`, `ANALYTICS_BEACON_PORT`, `ANALYTICS_APP_NAME`, `ANALYTICS_TIMEOUT_MS`). Voir la vision d'ensemble dans `MORFSYSTEM_ARCHITECTURE.md` (au niveau de l'écosystème).

# [1.9.1] - 2026-07-19
### Fixed
- **Monitoring UDP : logs de démarrage manquants (dont la vitesse SPI de la carte SD).** Le module UDP était initialisé tard (après le WiFi) et la tâche d'émission **jetait** les logs tant que le réseau n'était pas connecté ; tous les messages de boot (montage SD et fréquence retenue, init capteurs, etc.) étaient donc perdus pour le moniteur UDP. Désormais : la capture est installée **dès le début du `setup()`** (au plus tôt), les logs de démarrage sont **conservés dans une file tampon** (agrandie) puis **rejoués dans l'ordre dès la connexion WiFi**, sans être jetés (`src/utils/udp_logger.cpp`, `src/main.cpp`). Le moniteur UDP reçoit ainsi la totalité des logs applicatifs et cœur ESP à partir du `setup()` (seules les quelques lignes du bootloader ROM et de l'init du cœur Arduino, émises avant `setup()`, restent propres au port série physique).

# [1.9.0] - 2026-07-19
### Added
- **Monitoring des logs par UDP (sans câble série).** Nouveau module `src/utils/udp_logger.*` : les logs applicatifs (`addLog`) **et** ceux du cœur ESP-IDF/Arduino (WiFi, I2C, watchdog…) sont diffusés en UDP sur le réseau local, pour être suivis à distance (par ex. dans **Tabby**). L'émission passe par une **file FreeRTOS + une tâche dédiée** : le contexte d'origine d'un log (y compris la pile réseau) n'émet jamais lui-même de paquet, ce qui évite tout risque de réentrance/blocage. Les logs sont aussi désormais mis en miroir sur le port série (`Serial`). Configurable dans `include/config.h` : `UDP_LOG_ENABLED`, `UDP_LOG_PORT` (5005 par défaut), `UDP_LOG_HOST` (IP du PC récepteur ou `255.255.255.255` pour un broadcast). La réception côté PC et la configuration de Tabby sont décrites dans le guide utilisateur.

# [1.8.0] - 2026-07-19
### Changed
- **Affichage de l'historique nettement plus rapide (lecture SD optimisée).** Deux optimisations, sans perte de précision :
  - **Lecture séquentielle par blocs** : `queryRange()` et l'export CSV lisaient chaque mesure une par une (un `seek` + un `read` de 16 octets par enregistrement, soit ~1440 accès pour 24 h). Un nouveau parcours `forEachBinRecordFrom()` lit désormais les enregistrements contigus par blocs de 1 Ko (un seul `seek` initial, aucun `seek` par mesure), réduisant massivement le nombre d'accès SD.
  - **Cascade de fréquence SPI de la carte SD** : la carte était systématiquement montée à **1 MHz**. `SdManager` tente maintenant, en cascade décroissante, **20 → 10 → 4 → 1 MHz** et retient la plus haute fréquence qui fonctionne réellement (montage + création de dossier + test **écriture/relecture**). Une fréquence trop élevée qui « monte » mais lit mal est rejetée et l'on redescend d'un cran ; le formatage n'est jamais déclenché par un simple échec de vitesse. En cas de matériel marginal, le repli à 1 MHz garantit l'absence de régression.

# [1.7.1] - 2026-07-19
### Added
- **Page Historique : bascule « Temps réel ».** Une case à cocher (activée par défaut), placée à côté de « Synthèse », permet d'activer/désactiver le rafraîchissement automatique du graphe (`data/longterm.html`, `data/app.js`). Le rafraîchissement automatique reste par ailleurs limité aux périodes relatives ≤ 48 h sans comparaison.

# [1.7.0] - 2026-07-19
### Added
- **Page Statistiques : option de mise à jour en temps réel.** Une bascule « Mise à jour en temps réel » (activée par défaut) permet d'activer/désactiver le rafraîchissement automatique des tableaux (`data/stats.html`, `data/app.js`). Un bouton « Actualiser » permet un rafraîchissement manuel quand l'automatique est désactivé, et l'heure de dernière mise à jour est affichée.

# [1.6.3] - 2026-07-18
### Fixed
- **Page Statistiques : minima toujours à 0 (séries de valeurs aberrantes).** Le filtre temporel de `getRecentStats()` ne repérait qu'un pic **d'un seul point** ; or les échecs I2C répétés (avant le correctif 1.6.2) ont pu enregistrer **plusieurs mesures à 0 d'affilée**, que ce filtre laissait passer (le voisin étant aussi à 0). `getRecentStats()` utilise désormais un filtre **robuste médiane/MAD** par grandeur (`robustMetric`, `src/managers/history_manager.cpp`) : le seuil de rejet est piloté par la dispersion réelle des données (médiane ± 5·1,4826·MAD, avec un plancher), ce qui écarte les valeurs aberrantes **même en série** sans toucher aux variations normales. Les mesures enregistrées depuis la 1.6.2 étant déjà propres, les anciens zéros restants disparaissent aussi des statistiques au fil de leur sortie de la fenêtre 24 h.

# [1.6.2] - 2026-07-18
### Fixed
- **Stabilité de l'acquisition capteur / valeurs à zéro à la source.** La lecture (`src/modules/sensors.cpp`) ignorait le booléen de succès de `aht.getEvent()` et forçait `valid=true` : une erreur I2C (`i2cRead returned Error -1`) faisait enregistrer une mesure à `0`. Désormais :
  - le succès **et** la plausibilité de chaque lecture sont vérifiés (rejet des `NaN`, du `0 %` d'humidité, des valeurs hors plage) ; en cas d'échec, `valid=false` et l'historique n'enregistre rien (la minute est simplement sautée) ;
  - **jusqu'à 3 tentatives** par lecture (les erreurs I2C sont le plus souvent transitoires) ;
  - **récupération automatique du bus I2C** après 5 échecs consécutifs (`Wire.end()` + réinitialisation du bus et des capteurs) ;
  - la **dernière valeur valide** est renvoyée pour l'affichage temps réel quand une lecture échoue (au lieu d'afficher 0), avec `valid=false` ;
  - `Wire.setClock(100 kHz)`, `Wire.setTimeOut(50 ms)` (évite un blocage long si le bus se coince) et suréchantillonnage + filtre IIR du BMP280 (`setSampling`) pour des lectures plus stables.

# [1.6.1] - 2026-07-18
### Fixed
- **Valeurs aberrantes toujours visibles (graphe Historique) et statistiques à 0 incohérentes.** En 1.6.0, le filtrage n'agissait que côté client sur les points **déjà agrégés** (donc dilués, et sans traiter les bords), et la page **Statistiques** (`/api/stats` → `getRecentStats`) calculait min/max/moyenne sur les mesures **brutes** - d'où des minima à `0` / `-0.0` lorsqu'un capteur renvoie ponctuellement une valeur nulle. Le filtrage est désormais fait **au niveau des mesures brutes, côté serveur**, avant toute agrégation :
  - `queryRange()` applique un filtre anti-aberrations **en flux** (fenêtre glissante de 3 mesures) **par grandeur** : une mesure incohérente avec ses deux voisines est écartée de l'agrégation de la grandeur concernée (les autres restent valides). Les tranches sont désormais renvoyées avec une validité **par grandeur** (`temp`/`hum`/`pres` à `null` indépendamment) et l'API `/api/history` sérialise ces `null` séparément.
  - `getRecentStats()` (page Statistiques) écarte de la même façon les valeurs aberrantes par grandeur avant de calculer min/max/moyenne.
  - Le filtrage sur données brutes (échantillonnage 1 min) est bien plus efficace qu'au niveau des points agrégés : une valeur nulle isolée entre deux mesures normales est un pic évident. Le filtre client (`data/app.js`) est conservé comme second rempart. Les données brutes restent inchangées dans les fichiers.

# [1.6.0] - 2026-07-18
### Added
- **En-tête de fichier pour le format binaire (pérennité / compatibilité ascendante).** Chaque fichier `.bin` commence désormais par un en-tête `FileHeader` (`src/managers/history_manager.cpp`) : magic `"MTHB"`, version de format, taille d'en-tête, taille d'enregistrement, drapeaux de capteurs présents, nombre d'enregistrements et horodatages du premier/dernier relevé. MeteoHub identifie ainsi le format avant de lire et se déplace selon `recordSize` : de futurs capteurs (qualité de l'air, vent, UV…) pourront agrandir l'enregistrement **sans imposer de migrer** les anciens fichiers. La lecture (`probeBin`/`readBinRecordAt`) gère de façon transparente les fichiers avec en-tête **et** les fichiers sans en-tête de la v1.4.0 ; ces derniers sont convertis une fois lors du prochain enregistrement du jour (`upgradeLegacyBin`). La migration CSV et l'export s'appuient sur le même en-tête.
- **Détection des valeurs aberrantes à l'exploitation.** Un pic ou un creux d'un seul point (valeur incohérente au regard des relevés qui l'entourent, suivie d'un retour immédiat à la normale) est désormais **écarté du tracé des graphiques et des statistiques**, les points valides étant reliés directement (`filterOutliers`, `spanGaps`, `data/app.js`). La détection repose sur la **cohérence temporelle** (écart fort avec les deux voisins alors que ceux-ci restent cohérents entre eux), et non sur un seuil fixe ; un plancher de bruit par grandeur évite de nettoyer les micro-variations. Les **données brutes restent conservées** dans les fichiers : seule leur exploitation est adaptée. La synthèse de la page Historique est calculée sur ces mêmes séries filtrées (statistiques plus représentatives). Appliqué aussi au tableau de bord.

# [1.5.0] - 2026-07-18
### Added
- **Page « Système » (ex-« Mise à jour OTA »), hub de gestion.** La page (`data/system.html`) regroupe désormais, en plus de la mise à jour OTA :
  - **Luminosité de la NeoLED** : curseur 0-255 appliqué en direct et **persisté en NVS** (survit au redémarrage) - module `neopixel_status` (`neoSetBrightness`/`neoGetBrightness`, `Preferences`), API `GET`/`POST /api/led`.
  - **Export CSV de l'historique** : dernières 24 h / 7 j / 30 j / tout, en flux (`HistoryManager::exportCsv()`, lecture des `.bin`), API `GET /api/history/export.csv?from=&to=` avec en-tête de téléchargement. Le CSV redevient ainsi purement un format d'export.
  - **Export de la configuration** effective au format JSON - API `GET /api/config/export`.
  - **Accès aux outils** : liens vers le gestionnaire de fichiers et les logs.

### Changed
- **Menu principal réduit à 4 entrées : Tableau de bord, Statistiques, Historique, Système** (`data/menu.js`). Les pages **Fichiers** et **Logs** ne sont plus dans le menu : elles restent accessibles depuis la page Système.
- **Pied de page épuré** : suppression des icônes 💾 (Fichiers) et 📜 (Logs) - l'accès passe par la page Système (`data/footer.js`).
- L'ancienne URL `/ota.html` **redirige** vers `/system.html` (`src/managers/web_manager.cpp`) ; `data/ota.html` est supprimé.

# [1.4.0] - 2026-07-18
### Changed
- **Refonte du stockage de l'historique : format binaire journalier au lieu du CSV.** Le fonctionnement interne de MeteoHub ne repose plus sur des fichiers CSV mais sur un format binaire compact, mieux adapté à l'ESP32 et à un historique appelé à grandir pendant des années. Le CSV est conservé uniquement comme format d'export (lisible dans Excel/LibreOffice).
  - **Enregistrements de taille fixe (16 octets)** : `timestamp` (uint32) + température/humidité/pression (float), soit ~2× plus compact que le CSV, et surtout un **accès direct à une mesure par sa position** (recherche dichotomique) sans relire ce qui précède (`struct BinRecord`, `src/managers/history_manager.cpp`).
  - **Découpage par jour** : `/history/AAAA/MM/AAAA-MM-JJ.bin`. Une consultation 24 h ne lit qu'un fichier, une semaine sept. `queryRange()` saute directement (dichotomie) au premier enregistrement de la plage puis lit séquentiellement jusqu'à la borne - le coût ne dépend plus de la taille totale de l'historique.
  - **Fichiers de statistiques journalières `.bin` → `.stats`** : à côté de chaque `.bin`, un fichier `.stats` contient les valeurs déjà calculées au fil des acquisitions (min/max/moyenne de T°/Hu/Pression, nombre de mesures, première/dernière mesure et leurs horodatages). Mis à jour de façon incrémentale à chaque mesure (`DayStats`, `updateDayStats()`), ils permettent d'afficher la **synthèse quasi instantanément** sans relire les milliers de points du graphe.
  - **Nouvel endpoint `/api/history/summary?from=&to=`** : agrège les `.stats` de la plage et renvoie min/max/moyenne + variation par grandeur (`HistoryManager::querySynthesis()`). La synthèse de la page Historique l'utilise en priorité (extrêmes réels, non lissés par l'agrégation du graphe), avec repli sur un calcul côté client si aucun `.stats` n'est disponible.
  - **Migration automatique au démarrage** : les anciens fichiers `/history/AAAA-MM-JJ.csv` sont convertis une fois en `.bin` + `.stats`, puis renommés en `.csv.bak` pour ne pas être retraités. La lecture conserve un repli sur ces CSV tant qu'un `.bin` équivalent n'existe pas.
  - `readSdSampleNear()` (tendance 48 h de la page Statistiques) et `clearHistory()` (suppression récursive de l'arborescence) sont adaptés au nouveau format.

# [1.3.2] - 2026-07-18
### Added
- **Synthèse Historique : écart A ↔ B en comparaison.** Lorsqu'une période de comparaison est active, chaque carte de synthèse affiche en plus l'écart des moyennes `A − B` (avec flèche/couleur), pour chiffrer d'un coup d'œil de combien la période principale est plus chaude/humide/haute en pression que la période comparée (`data/app.js`, `data/style.css` : `.synth-compare`).

# [1.3.1] - 2026-07-18
### Fixed
- **Page Historique : icône du calendrier des champs de dates peu visible.** L'icône native du sélecteur `datetime-local` restait sombre sur le fond foncé. Ajout de `color-scheme: dark` (et d'un filtre `invert` de repli sur `::-webkit-calendar-picker-indicator`) pour la rendre claire et lisible (`data/style.css`).

# [1.3.0] - 2026-07-18
### Added
- **Page Historique : ligne de synthèse optionnelle au-dessus du graphe.** Une bascule « Synthèse » (masquée par défaut) affiche, pour la période sélectionnée, un résumé calculé automatiquement par grandeur (température, humidité, pression) : variation sur la période (dernier − premier point, avec flèche ▲/▼/= et couleur), minimum, maximum et moyenne. Objectif : savoir d'un coup d'œil si la période a été stable, si un front est passé, si l'humidité s'est effondrée, sans analyser les courbes. Le calcul est effectué côté client (`data/app.js`, `computeSynthesis()` / `renderSynthesis()`) à partir des points déjà renvoyés par `/api/history` - aucun nouvel endpoint. La synthèse porte sur la période principale (A) ; activer/désactiver la bascule redessine sans relancer de requête (`data/longterm.html`, `data/style.css` : `.synth-panel`).

# [1.2.2] - 2026-07-18
### Changed
- **Page Historique : couleurs des courbes de comparaison.** Les courbes de la période comparée (B) reprennent désormais les mêmes couleurs que les courbes principales (température `#00a8ff`, humidité `#00ff88`, pression `#ff00ff`) et ne se distinguent plus que par leur tracé en pointillés (`data/app.js`, `COMPARE_COLORS`).

# [1.2.1] - 2026-07-18
### Fixed
- **Reboot en boucle à la consultation de l'historique (task watchdog `async_tcp`).** La lecture des fichiers CSV sur la carte SD par `HistoryManager::queryRange()` s'exécute dans la tâche `async_tcp` (handler ESPAsyncWebServer), laquelle est surveillée par le task watchdog (`CONFIG_ASYNC_TCP_USE_WDT`). L'ancien yield coopératif (`delay(0)`) ne réarmait pas ce watchdog : une lecture un peu longue déclenchait un abort puis un redémarrage en boucle. `cooperativeYieldEvery()` (`src/utils/cooperative_yield.h`) appelle désormais `esp_task_wdt_reset()` avant de céder la main (protège aussi `readSdSampleNear()` via `/api/stats`), et la boucle de lecture SD de `queryRange()` cède la main plus souvent (toutes les 32 lignes). Côté web (`data/app.js`), le rafraîchissement automatique de la page Historique est en outre limité aux périodes ≤ 48 h pour éviter de relancer en continu un scan de plusieurs fichiers CSV.

# [1.2.0] - 2026-07-18
### Added
- **Page « Historique » (ex-« Historique 24h ») : sélection et comparaison de périodes.** La page (`data/longterm.html`, `data/app.js`) permet désormais de choisir la fenêtre affichée - dernières 24 h / 48 h / 7 jours / 30 jours, « aujourd'hui », ou une plage personnalisée (`du`/`au` via des champs date-heure) - et de comparer deux périodes de même durée (aucune, « période précédente », ou « autre période… »). La période B est superposée en traits pointillés et alignée par index sur la période A ; une ligne d'information rappelle les plages exactes affichées. Les sélecteurs de période et de comparaison sont regroupés dans une barre en haut de page ; toute modification est appliquée automatiquement (pas de bouton « Afficher »), les champs de dates personnalisés ne s'affichant que lorsque l'option correspondante est choisie.
- **Cartouche « Chargement en cours… »** centré sur le graphe pendant la récupération des données de la page Historique (`#chartLoading`, `data/style.css`).
- **API `/api/history` : mode plage absolue `?from=<unix>&to=<unix>[&interval=<s>]`.** Nouvelle méthode `HistoryManager::queryRange()` (`src/managers/history_manager.cpp`) qui agrège les mesures sur une plage temporelle arbitraire, en lisant en priorité les fichiers CSV journaliers de la carte SD (`/history/AAAA-MM-JJ.csv`, potentiellement sur plusieurs jours) puis en complétant par l'historique RAM le plus récent non encore écrit sur SD (ou l'intégralité en l'absence de carte SD). Le nombre de tranches est borné (800 max) pour maîtriser la mémoire, l'intervalle étant élargi automatiquement si besoin. Les tranches sans donnée sont émises avec des valeurs `null` afin de conserver l'alignement des index entre deux périodes comparées. Le mode fenêtre glissante historique (`window`/`interval`/`points`) reste inchangé et rétrocompatible (utilisé par le tableau de bord).

### Changed
- **Graphe d'historique : légendes raccourcies.** Les libellés des trois séries deviennent `°C`, `Hu%` et `hPa` (au lieu de « Température (°C) », « Humidité (%) », « Pression (hPa) »), plus lisibles sur mobile. En comparaison, les séries de la période B portent le suffixe `(B)`.
- **Contrôle d'échelle « Élargissement » renommé « Zoom » et sémantique inversée.** Le curseur va désormais de 0 à 100 % : à `0 %` l'échelle correspond aux min/max fixes configurés (la courbe apparaît quasiment plate/unique), à `100 %` l'échelle épouse exactement l'amplitude des données (la courbe occupe toute la hauteur). En interne (`getDynamicMinMax`, mode « Mixte »), l'ancienne marge additive est remplacée par une interpolation linéaire entre l'échelle complète et l'amplitude dynamique. Valeur par défaut spécifique à chaque page, portée par l'attribut `value` du curseur : `90 %` sur le tableau de bord (`data/index.html`), `75 %` sur la page Historique (`data/longterm.html`). Le calcul d'échelle (`updateChartScale`) prend en compte l'ensemble des séries affichées, y compris la période comparée.
- **Entrée de menu « Historique 24h » renommée « Historique »** (`data/menu.js`, titre et en-tête de `data/longterm.html`).

# [1.1.5] - 2026-06-23
### Added
- **Tendance météo sur 1h/12h/24h/48h** (page Statistiques) : `HistoryManager::getTrend()` calcule désormais le delta et la direction (hausse/baisse/stable) de la température, l'humidité et la pression sur quatre fenêtres temporelles au lieu de deux (1h et 24h auparavant). La fenêtre 12h est dérivée de l'historique RAM (24h disponibles à ~1 point/min) ; la fenêtre 48h est récupérée en lisant le fichier CSV journalier de J-2 sur la carte SD (`/history/AAAA-MM-JJ.csv`, nouvelle méthode `HistoryManager::readSdSampleNear()`) et n'est disponible que si une carte SD avec historique est présente (flag `available_48h`, affiché « N/D » sinon).
- **Tendance générale plus fiable** : `computeGlobalTrendLabelFr()` (`src/managers/web_manager.cpp`) croise désormais la direction de la pression sur les fenêtres 1h/12h/24h(/48h) pour distinguer une vraie tendance de fond (« Amélioration durable », « Dégradation durable ») d'une simple fluctuation court terme, en plus des signaux rapides déjà existants (chute brutale de pression + humidité en hausse, etc.).
- Page Statistiques (`data/stats.html`, `data/app.js`) : le tableau « Tendance météo » affiche désormais 4 colonnes (1h/12h/24h/48h) avec flèches de direction, et la tendance générale est affichée séparément sous le tableau.

# [1.1.4] - 2026-06-23
### Fixed
1. **Page Statistiques : tendances toujours à zéro/"stable"**
   - **Problème** : Le endpoint `/api/stats` (`src/managers/web_manager.cpp`) calculait correctement les tendances (`HistoryManager::getTrend()`) mais ne sérialisait dans la réponse JSON que `trend.global_label_fr`. Les sous-objets `trend.temp`, `trend.hum` et `trend.pres` attendus par le frontend (`data/app.js`, fonction `fetchStats`) n'existaient jamais dans la réponse, donc les deltas 1h/24h affichaient toujours `0.0` et les directions toujours `stable`, empêchant de dégager une tendance.
   - **Solution** : Ajout de la sérialisation complète de `trend.temp`, `trend.hum` et `trend.pres` (`delta_1h`, `delta_24h`, `direction_1h`, `direction_24h`) dans la réponse de `/api/stats`. Augmentation de la taille du buffer JSON (`DynamicJsonDocument`) de 2048 à 3072 octets pour accueillir les champs supplémentaires.
   - **Fichier** : `src/managers/web_manager.cpp`.

2. **Page Fichiers : téléchargement impossible dans les sous-dossiers**
   - **Problème** : Le endpoint `/api/files/list` renvoyait le chemin complet du fichier (`file.path()`, ex. `/sous-dossier/fichier.txt`) dans le champ JSON `"name"`, alors que le frontend (`data/files.js`) traite ce champ comme un simple nom de fichier et reconstruit le chemin complet en le concaténant avec le dossier courant. Résultat : un chemin dupliqué (ex. `/sous-dossier/sous-dossier/fichier.txt`) introuvable côté serveur, d'où l'échec systématique du téléchargement (et de la suppression) pour tout fichier non situé à la racine.
   - **Solution** : Utilisation de `file.name()` (nom de base uniquement) au lieu de `file.path()` pour le champ `"name"` de la liste de fichiers.
   - **Fichier** : `src/managers/web_manager.cpp`.

# [1.1.3] - 2026-03-15
### Fixed
- **Affichage corrompu du menu lors de la sélection des items** : Ajout d'un `d->clear()` au début du bloc de rendu du menu dans `UiManager::drawPage()`. Lors de la navigation dans le menu, l'écran n'était pas effacé avant le redessin car `screen_context_changed` était `false` (le mode menu n'avait pas changé). Les anciens items se superposaient aux nouveaux, provoquant un affichage corrompu.

# [1.1.2] - 2026-03-08
### Fixed
Correction critique de la corruption du système de fichiers et erreurs de compilation associées.

1. **Ajout explicite de `flush()` avant fermeture de fichier**
   - **Problème** : Les données restaient dans le cache RAM et n'étaient pas écrites physiquement sur la carte SD/LittleFS avant la fermeture, causant une corruption de la table FAT en cas d'écriture rapide ou de coupure.
   - **Solution** : Ajout systématique de `file.flush()` avant chaque `file.close()` dans `history_manager.cpp` (fonctions `saveRecent`, `saveToSd`) et `sd_manager.cpp` (fonction `verifyWriteAccess`).
   - **Impact** : Garantit l'intégrité des fichiers CSV et binaires après chaque écriture.

2. **Correction du type de retour de `flush()`**
   - **Problème** : Erreur de compilation "l'expression doit avoir le type booléen" car `file.flush()` retourne `void` sur certaines versions du core ESP32, mais le code tentait de l'évaluer dans un `if`.
   - **Solution** : Suppression des tests conditionnels `if (!file.flush())`. La fonction est maintenant appelée de manière impérative.
   - **Fichiers** : `src/managers/sd_manager.cpp`, `src/managers/history_manager.cpp`.

3. **Ajout de l'inclusion manquante `cooperative_yield.h`**
   - **Problème** : Erreur de compilation "identificateur non défini" pour la macro `COOPERATIVE_YIELD_EVERY` dans `history_manager.cpp`.
   - **Solution** : Ajout de `#include "../utils/cooperative_yield.h"` en tête de fichier.

4. **Implémentation de Mutex pour la protection des écritures**
   - **Problème** : Risque de corruption si deux tâches (ex: sauvegarde historique et test web) écrivent simultanément sur la SD.
   - **Solution** : Ajout d'un `std::mutex` dans `SdManager` et utilisation de `std::lock_guard` dans les méthodes critiques (`verifyWriteAccess`, `ensureHistoryDirectory`, `openFileSafe`).

5. **Simplification de la logique de montage SD**
   - **Problème** : Échecs de montage à 10MHz et 4MHz sur cartes sensibles.
   - **Solution** : Fréquence unique fixée à 1 MHz (1000000 Hz) pour une stabilité maximale, supprimant les tentatives multi-fréquences inutiles.

# [1.1.1] - 2026-03-08
- Correction erreur de compilation dans `SdManager::resetSpiBus()` : la méthode `SPIClass::begin()` retournant `void` sur ESP32, la capture du retour booléen a été supprimée.
- Simplification de la logique de montage SD : tentative unique à 1 MHz pour maximiser la stabilité.
- Augmentation des délais d'initialisation SPI pour garantir la stabilité électrique lors du montage.

# [1.1.0] - 2026-03-08
- Refonte complète du `WebManager` pour utiliser exclusivement `std::string` (C++ Standard).
- Conversion explicite aux frontières entre les types Arduino (`String`) et C++ (`std::string`).
- Support complet de la gestion de fichiers (upload, download, suppression) et OTA.

# [1.0.181] - 2026-03-07
- Durcissement anti read-only: `SdManager::begin()` et `ensureMounted()` n'annoncent plus la SD disponible si création `/history` ou test d'écriture échoue.
- `ensureHistoryDirectory()` retourne désormais un booléen et tente un fallback `/sd/history` pour les cas de mountpoint atypiques.
- En cas d'échec d'écriture après format/remount, la SD est démontée et marquée indisponible pour éviter les erreurs répétées côté sauvegarde historique.

# [1.0.180] - 2026-03-07
- Ajout d'un fallback local `SD_DET_ACTIVE_LEVEL` dans `sd_manager.cpp` pour éviter toute erreur de symbole non défini selon l'ordre d'includes/toolchain.
- Maintien de `isCardDetected()` déclaré+défini dans `SdManager` avec check non bloquant au boot.

# [1.0.179] - 2026-03-07
- Réintroduction explicite de `isCardDetected()` dans `SdManager` pour lever l'erreur "identificateur non défini" signalée à la compilation/IDE.
- Vérification DET conservée non bloquante au démarrage SD (log diagnostic sans empêcher les tentatives de montage).

# [1.0.178] - 2026-03-07
- Correction supplémentaire de `SdManager::verifyWriteAccess()` pour supprimer définitivement les erreurs de parsing C++ (bloc unique, retours explicites, suppression fichier test centralisée).
- Réordonnancement des includes dans `sd_manager.cpp` (`Arduino.h` avant les logs) pour éviter les effets de bord de macro selon toolchain.

# [1.0.177] - 2026-03-07
- Correction de compilation dans `SdManager::verifyWriteAccess()` avec réécriture plus explicite de la séquence d'ouverture/écriture/fermeture du fichier test SD.
- Ajout de `#include <Arduino.h>` dans `sd_manager.cpp` pour fiabiliser la compilation croisée des types Arduino (`size_t`, API runtime) selon toolchain.

# [1.0.176] - 2026-03-07
- Réécriture complète du `SdManager` sur la méthode validée "mode stable 10MHz" : instance `FSPI` dédiée recréée avant chaque montage et `SD.begin(..., format_if_fail=...)`.
- Suppression de la dépendance bloquante à la broche DET dans la logique de montage pour éviter les faux négatifs de détection.
- Formatage robuste aligné sur le code de référence: remount à 10MHz avec `format_if_fail=true` puis test d'écriture critique.
- Conservation des fonctionnalités projet liées à la SD (historique `/history`, sauvegarde, lecture, upload/suppression via APIs existantes).

# [1.0.175] - 2026-03-07
- Renforcement du montage SD sur ESP32-S3: ajout d'essais à 1MHz et 400kHz en plus des fréquences rapides.
- Préparation explicite du bus SPI avant `SD.begin` (CS HIGH, MISO pull-up, clocks d'amorçage) pour améliorer la compatibilité des cartes/modules sensibles.
- Ajustement `max_files` lors du montage SD à 10 pour limiter les échecs liés aux ouvertures de fichiers simultanées.

# [1.0.174] - 2026-03-07
- Correction SD_DET: la détection de carte n'est plus bloquante pour le montage (certains modules ont une polarité inversée ou un signal bruité).
- Ajout d'un échantillonnage multi-lectures de la broche DET avec logs détaillés (LOW/HIGH) pour diagnostiquer le câblage réel.
- Ajout du paramètre `SD_DET_ACTIVE_LEVEL` (LOW/HIGH) dans `board_config.h` pour s'adapter aux lecteurs à polarité inversée.
- Si la carte est déjà montée, un état DET incohérent n'entraîne plus de démontage forcé; seule la vérification `SD.cardType()` décide de la disponibilité.

# [1.0.173] - 2026-03-07
- Refonte du gestionnaire SD pour s'aligner sur la méthode validée (SPI FSPI dédié + `SD.begin(..., format_if_fail=true)`).
- Respect strict du mapping défini dans `board_config.h` (CLK=9, D0/MISO=10, CMD/MOSI=11, D3/CS=12, DET=14).
- Ajout d'une détection de présence carte via `SD_DET_PIN` (LOW=présente) avant montage/réessais.
- Conservation des fonctionnalités SD existantes: lecture/écriture, incrémentation quotidienne des fichiers CSV d'historique, suppression/upload web et formatage.

# [1.0.172] - 2026-02-25
- Ajout et liens croisés de la documentation débutant (EN/FR) dans tous les documents utilisateur.
- Tous les guides, FAQ, configuration et index référencent désormais l'onboarding débutant.
- Version minimale valide : 1.0.172

# [1.0.171] - 2026-02-25
- Ajout de la gestion avancée des échelles pour les graphiques température, humidité et pression.
- Trois modes disponibles : fixe, dynamique, mixte (avec élargissement configurable).
- Contrôles interactifs sur l'UI web pour choisir le mode et le pourcentage.
- Aide contextuelle sous le graphique.
- Synchronisation automatique entre config.h et l'UI web.

# [1.0.170] - 2026-02-24
### Corrigé
- Application du même schéma de zone sûre que les graphes aux autres pages OLED.
- Conservation des titres dans la bande haute et déplacement du début de contenu prévisions/logs sous la zone haute réservée SSD1306.
- Ajustement de l'espacement des lignes de logs pour éviter le chevauchement haut sur les SSD1306 à bande jaune.
