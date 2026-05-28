#ifndef _INDOOR_UWB_TEST_CONFIG_H
#define _INDOOR_UWB_TEST_CONFIG_H

/**
 * Configuración para tests PlatformIO (Unity).
 * Credenciales WiFi: mismas que src/config.h; requiere AP disponible para test_wifi.
 */

#include <Arduino.h>
#include <IPAddress.h>

#ifndef INDOOR_UWB_ROLE_TAG
#define INDOOR_UWB_ROLE_TAG 1
#endif

#ifndef INDOOR_UWB_BOARD_MAKERFABS_V1
#define INDOOR_UWB_BOARD_MAKERFABS_V1 1
#endif

#include "../../src/config.h"

/** IP de prueba (tag); evita colisión si subes firmware tag en .200 */
#define TEST_WIFI_LOCAL_IP IPAddress(192, 168, 1, 199)

/** 1 = comprobar MAC personalizada (definida en test_config.cpp) */
#define WIFI_TEST_CUSTOM_MAC 0

#if WIFI_TEST_CUSTOM_MAC
extern uint8_t test_mac_address[6];
#endif

#endif
