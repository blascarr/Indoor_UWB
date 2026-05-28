#ifndef _INDOOR_UWB_WIFI_TEST_HELPER_H
#define _INDOOR_UWB_WIFI_TEST_HELPER_H

#include "../src/test_config.h"
#include <WiFi.h>

bool connectTestWifi();
extern bool wifiTestConnected;

#if WIFI_TEST_CUSTOM_MAC
void applyTestMacAddress();
#endif

#endif
