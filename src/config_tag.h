#ifndef _INDOOR_UWB_CONFIG_TAG_H
#define _INDOOR_UWB_CONFIG_TAG_H

#define DEVICE_NAME "RoboTag"
#define WIFI_HOST "INDOORUWB"
#define LOCAL_IP IPAddress(192, 168, 1, 200)
#define OTA_HOST "INDOOR"
static char DW1000_ADDRESS[] = "7D:00:22:EA:82:60:3B:9C";
static const uint8_t MACAddress[] = {0x40, 0x91, 0x51, 0xAE, 0x95, 0x50};
static const uint8_t ESPNOW_BROADCAST[] = {0x40, 0x91, 0x51, 0xAE, 0x95, 0x74};
#define IMU_ENABLED 0

#endif
