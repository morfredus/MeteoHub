#include "calibration_store.h"
#include <Preferences.h>

// Espace et clés NVS (limite NVS : 15 caractères par nom).
static const char* NVS_NS      = "sensor_cal";
static const char* KEY_IN_T    = "in_t";
static const char* KEY_IN_H    = "in_h";
static const char* KEY_OUT_T   = "out_t";
static const char* KEY_OUT_H   = "out_h";
static const char* KEY_IN_P    = "in_p";
static const char* KEY_OUT_P   = "out_p";
static const char* KEY_IN_ALT  = "in_alt";
static const char* KEY_OUT_ALT = "out_alt";

CalibrationSet loadCalibration() {
    CalibrationSet set;
    Preferences prefs;
    // Lecture seule : un espace absent (hub jamais calibré) laisse 0 / 0.
    if (prefs.begin(NVS_NS, true)) {
        set.indoor.temperature  = prefs.getFloat(KEY_IN_T, 0.0f);
        set.indoor.humidity     = prefs.getFloat(KEY_IN_H, 0.0f);
        set.outdoor.temperature = prefs.getFloat(KEY_OUT_T, 0.0f);
        set.outdoor.humidity    = prefs.getFloat(KEY_OUT_H, 0.0f);
        set.indoor.pressure     = prefs.getFloat(KEY_IN_P, 0.0f);
        set.outdoor.pressure    = prefs.getFloat(KEY_OUT_P, 0.0f);
        set.indoor.altitude     = prefs.getFloat(KEY_IN_ALT, 0.0f);
        set.outdoor.altitude    = prefs.getFloat(KEY_OUT_ALT, 0.0f);
        prefs.end();
    }
    set.indoor = mhcal::sanitize(set.indoor);
    set.outdoor = mhcal::sanitize(set.outdoor);
    return set;
}

CalibrationSet saveCalibration(CalibrationSet set) {
    set.indoor = mhcal::sanitize(set.indoor);
    set.outdoor = mhcal::sanitize(set.outdoor);
    Preferences prefs;
    if (prefs.begin(NVS_NS, false)) {
        prefs.putFloat(KEY_IN_T, set.indoor.temperature);
        prefs.putFloat(KEY_IN_H, set.indoor.humidity);
        prefs.putFloat(KEY_OUT_T, set.outdoor.temperature);
        prefs.putFloat(KEY_OUT_H, set.outdoor.humidity);
        prefs.putFloat(KEY_IN_P, set.indoor.pressure);
        prefs.putFloat(KEY_OUT_P, set.outdoor.pressure);
        prefs.putFloat(KEY_IN_ALT, set.indoor.altitude);
        prefs.putFloat(KEY_OUT_ALT, set.outdoor.altitude);
        prefs.end();
    }
    return set;
}
