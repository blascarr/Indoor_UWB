#ifndef _INDOOR_UWB_CONFIG_ANCHOR_H
#define _INDOOR_UWB_CONFIG_ANCHOR_H

#define DEVICE_NAME "Indoor_D"
#define WIFI_HOST "Indoor_D"
#define LOCAL_IP IPAddress(192, 168, 1, 210)
#define OTA_HOST "ANCHOR_D"
static char ANCHOR_ADDRESS[] = "83:17:5B:D5:A9:9A:E2:9D";
static const uint8_t MACAddress[] = {0x40, 0x91, 0x51, 0xAE, 0x95, 0x74};
static const uint8_t ESPNOW_BROADCAST[] = {0x40, 0x91, 0x51, 0xAE, 0x95, 0x50};
#define IMU_ENABLED 0

#endif
