#ifndef _INDOOR_UWB_CONFIG_ANCHOR_H
#define _INDOOR_UWB_CONFIG_ANCHOR_H

#ifndef DEVICE_NAME
#define DEVICE_NAME "Indoor_D"
#endif
#ifndef WIFI_HOST
#define WIFI_HOST "Indoor_D"
#endif
#if defined(INDOOR_UWB_IP_O4)
#define LOCAL_IP INDOOR_UWB_MAKE_IP(INDOOR_UWB_IP_O4)
#else
#define LOCAL_IP INDOOR_UWB_MAKE_IP(211)
#endif
#ifndef OTA_HOST
#define OTA_HOST "ANCHOR_D"
#endif
#ifndef ANCHOR_UWB_ADDRESS
#define ANCHOR_UWB_ADDRESS "84:15:5B:D5:A9:9A:E2:9E"
#endif
static char ANCHOR_ADDRESS[] = ANCHOR_UWB_ADDRESS;

#if defined(MAC_ADDR_0)
static const uint8_t MACAddress[] = {MAC_ADDR_0, MAC_ADDR_1, MAC_ADDR_2,
									   MAC_ADDR_3, MAC_ADDR_4, MAC_ADDR_5};
#else
static const uint8_t MACAddress[] = {0x40, 0x91, 0x51, 0xAE, 0x95, 0x75};
#endif

#if defined(ESPNOW_TAG_MAC_0)
static const uint8_t ESPNOW_TAG_MAC[] = {
	ESPNOW_TAG_MAC_0, ESPNOW_TAG_MAC_1, ESPNOW_TAG_MAC_2,
	ESPNOW_TAG_MAC_3, ESPNOW_TAG_MAC_4, ESPNOW_TAG_MAC_5};
#else
static const uint8_t ESPNOW_TAG_MAC[] = {0x40, 0x91, 0x51, 0xAE, 0x95, 0x50};
#endif

#define IMU_ENABLED 0

#endif
