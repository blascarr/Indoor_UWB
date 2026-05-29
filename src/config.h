#ifndef _INDOOR_UWB_CONFIG_H
#define _INDOOR_UWB_CONFIG_H

#include <Arduino.h>
#include <IPAddress.h>
#include "config_network.h"

// ------------------ Placa / pinout DW1000 ------------------ //
#if defined(INDOOR_UWB_BOARD_MAKERFABS_PRO_DISPLAY)
/** Makerfabs ESP32 UWB Pro con pantalla SSD1306 (SS en 21). */
#define UWB_SPI_SCK 18
#define UWB_SPI_MISO 19
#define UWB_SPI_MOSI 23
#define UWB_PIN_RST 27
#define UWB_PIN_IRQ 34
#define UWB_PIN_SS 21
#elif defined(INDOOR_UWB_BOARD_MAKERFABS_V1) ||                                \
	defined(INDOOR_UWB_BOARD_ESP32DEV)
/** Makerfabs ESP32 UWB v1.0 / Basic / Pro sin display; esp32dev mismo pinout.
 */
#define UWB_SPI_SCK 18
#define UWB_SPI_MISO 19
#define UWB_SPI_MOSI 23
#define UWB_PIN_RST 27
#define UWB_PIN_IRQ 34
#define UWB_PIN_SS 4
#else
#define UWB_SPI_SCK 18
#define UWB_SPI_MISO 19
#define UWB_SPI_MOSI 23
#define UWB_PIN_RST 27
#define UWB_PIN_IRQ 34
#define UWB_PIN_SS 4
#endif

// ------------------ WiFi común ------------------ //
#define WIFI_SSID "ZMS"
#define WIFI_PASS "ZM4K3RS:P"
#define GATEWAY INDOOR_UWB_GATEWAY
#define SUBNET INDOOR_UWB_SUBNET
#define PRIMARYDNS IPAddress(9, 9, 9, 9)
#define SECONDARYDNS IPAddress(208, 67, 222, 222)
#define WIFIMANAGER_ENABLED 0
#define WIFI_SUPERVISOR_ENABLED 1
#define WIFI_SUPERVISOR_POLL_MS 250u
#define WIFI_CONNECT_ATTEMPTS_PER_CYCLE 10
#define WIFI_ATTEMPT_GAP_MS 150u
#define WIFI_JOIN_TIMEOUT_PER_ATTEMPT_MS 2500u
#define WIFI_JOIN_FIRST_ATTEMPT_MS 15000u
#define WIFI_RETRY_CYCLE_MS (60UL * 1000UL)

// ------------------ OTA ------------------ //
#define OTA_ENABLED 1
#define OTA_PORT 8266

// ------------------ UWB / trilateración (tag) ------------------ //
#define UWB_DEBUG 1
/** 1 = MODE_LONGDATA_RANGE_ACCURACY; 0 = MODE_LONGDATA_RANGE_LOWPOWER */
#define UWB_RF_MODE_ACCURACY 1
#define UWB_RANGE_FILTER 1
#ifndef UWB_RANGE_FILTER_SIZE
#define UWB_RANGE_FILTER_SIZE 10
#endif
#ifndef UWB_REPLY_DELAY_US
#define UWB_REPLY_DELAY_US 12000u
#endif
#ifndef UWB_TIMER_DELAY_MS
#define UWB_TIMER_DELAY_MS 120u
#endif
#ifndef UWB_RESET_PERIOD_MS
#define UWB_RESET_PERIOD_MS 500u
#endif
#ifndef UWB_RANGE_MIN_M
#define UWB_RANGE_MIN_M 0.05f
#endif
#ifndef UWB_RANGE_MAX_M
#define UWB_RANGE_MAX_M 30.0f
#endif
#ifndef UWB_RANGE_WARMUP_SAMPLES
#define UWB_RANGE_WARMUP_SAMPLES 5u
#endif
/** Corrección por defecto (m). Negativo resta a la lectura: corregida = raw + offset */
#ifndef UWB_DEFAULT_OFFSET_M
#define UWB_DEFAULT_OFFSET_M (-0.20f)
#endif
#define TRILATERATION_DEBUG 1
#define TRILATERATION_NODES 3
#ifndef TRILATERATION2D
#define TRILATERATION3D 1
#endif

// ------------------ Anchors / almacenamiento ------------------ //
#define EEPROM_ANCHORLIST_SIZE 4
#define ANCHORLIST_FINGERPRINT '@'

#if defined(INDOOR_UWB_ROLE_TAG)
#define PREFS_NAMESPACE "uwb_tag"
#define PREFS_KEY_ANCHOR_LIST "anchors"
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
#define PREFS_NAMESPACE "uwb_anchor"
#define PREFS_KEY_POS_X "pos_x"
#define PREFS_KEY_POS_Y "pos_y"
#define PREFS_KEY_POS_Z "pos_z"
#define PREFS_KEY_POS_OFFSET "pos_offset"
#define PREFS_KEY_UWB_ADDRESS "uwb_addr"
#define UWB_EUI_STRING_LEN 24
#endif

// ------------------ ESP-NOW ------------------ //
static const uint8_t ESPNOW_BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF,
											   0xFF, 0xFF, 0xFF};

// ------------------ Servidor HTTP ------------------ //
#define SERVER_PORT 80

// ------------------ UWB ranging (TickerFree, tag y anchor) ------------------ //
#define UWB_LOOP_INTERVAL_MS 30u

// ------------------ Serial debug ------------------ //
#define PRINTDEBUG 1
#define SERIALDEBUG Serial

#if PRINTDEBUG
#define DUMPS(s) SERIALDEBUG.print(F(s))
#define DUMPSLN(s) SERIALDEBUG.println(F(s))
#define DUMPV(v) SERIALDEBUG.print(v)
#define DUMPVHEX(v) SERIALDEBUG.print(v, HEX)
#define DUMPVLN(v) SERIALDEBUG.println(v)
#define DUMPPRINTLN() SERIALDEBUG.println()
#define DUMP(s, v)                                                             \
	do {                                                                       \
		DUMPS(s);                                                              \
		DUMPV(v);                                                              \
	} while (0)
#define DUMPLN(s, v)                                                           \
	do {                                                                       \
		DUMPS(s);                                                              \
		DUMPV(v);                                                              \
		DUMPPRINTLN();                                                         \
	} while (0)
#define DUMPHEX(s, v)                                                          \
	do {                                                                       \
		DUMPS(s);                                                              \
		DUMPVHEX(v);                                                           \
	} while (0)
#define DUMPF(fmt, ...) SERIALDEBUG.printf(fmt, ##__VA_ARGS__)
#else
#define DUMPS(s)
#define DUMPSLN(s)
#define DUMPV(v)
#define DUMPVHEX(v)
#define DUMPVLN(v)
#define DUMPPRINTLN()
#define DUMP(s, v)
#define DUMPLN(s, v)
#define DUMPHEX(s, v)
#define DUMPF(fmt, ...)
#endif

#if defined(INDOOR_UWB_ROLE_TAG)
#include "config_tag.h"
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
#include "config_anchor.h"
#else
#error "Definir INDOOR_UWB_ROLE_TAG o INDOOR_UWB_ROLE_ANCHOR en build_flags"
#endif

#endif
