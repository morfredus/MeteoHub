#pragma once
#include "display_interface.h"
#include "../managers/wifi_manager.h"
#include "sensors.h"
#include "../managers/forecast_manager.h"
#include "../managers/history_manager.h"

void pageNetwork_oled(DisplayInterface& d, WifiManager& wifi, int pageIndex, int pageCount);
void pageSystem_oled(DisplayInterface& d, int pageIndex, int pageCount);
void pageSensor_oled(DisplayInterface& d, HistoryManager& history, int pageIndex, int pageCount);
void pageLogs_oled(DisplayInterface& d, int pageIndex, int pageCount, int scrollOffset = 0);
void pageWeather_oled(DisplayInterface& d, SensorManager& sensors, ForecastManager& forecast, HistoryManager& history, int pageIndex, int pageCount);
void pageGraph_oled(DisplayInterface& d, HistoryManager& history, int type, int pageIndex, int pageCount, bool outdoor = false);
void pageForecast_oled(DisplayInterface& d, ForecastManager& forecast, int view, int pageIndex, int pageCount);
void drawSplashScreen_oled(DisplayInterface& d);
void drawBootProgress_oled(DisplayInterface& d, int step, int total, const std::string& msg);
//