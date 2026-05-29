#ifndef _INDOOR_UWB_ANCHOR_MODEL_H
#define _INDOOR_UWB_ANCHOR_MODEL_H

#include "../config.h"
#include <Arduino.h>

inline float roundMeters(float value, int decimals) {
	if (decimals < 0) {
		return value;
	}
	double factor = 1.0;
	for (int i = 0; i < decimals; i++) {
		factor *= 10.0;
	}
	return (float)(round((double)value * factor) / factor);
}

struct Anchor {
	char name[10];
	char DW1000_Address[20];
	char MAC_Address[18];
	uint16_t shortAddress;
	float x;
	float y;
	float z;
	float o;
};

class AnchorList {
  public:
	uint8_t devices = 0;
	char fingerprint = ANCHORLIST_FINGERPRINT;
	Anchor list[EEPROM_ANCHORLIST_SIZE];

	AnchorList() = default;

	static void formatMac(const uint8_t mac[6], char *out, size_t outLen) {
		snprintf(out, outLen, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1],
				 mac[2], mac[3], mac[4], mac[5]);
	}

	static bool parseMac(const char *str, uint8_t mac[6]) {
		if (str == nullptr || str[0] == '\0') {
			return false;
		}
		unsigned int values[6];
		if (sscanf(str, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2],
				   &values[3], &values[4], &values[5]) != 6) {
			return false;
		}
		for (int i = 0; i < 6; i++) {
			mac[i] = (uint8_t)values[i];
		}
		return true;
	}

	/** Dirección extendida DW1000 (8 bytes, formato AA:BB:...:HH). */
	static bool parseUwbAddress(const char *str, char *out, size_t outLen) {
		if (str == nullptr || str[0] == '\0') {
			return false;
		}
		unsigned int values[8];
		if (sscanf(str, "%x:%x:%x:%x:%x:%x:%x:%x", &values[0], &values[1],
				   &values[2], &values[3], &values[4], &values[5], &values[6],
				   &values[7]) != 8) {
			return false;
		}
		snprintf(out, outLen,
				 "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", values[0],
				 values[1], values[2], values[3], values[4], values[5], values[6],
				 values[7]);
		return true;
	}

	static bool isUwbAddressFormat(const char *str) {
		unsigned int v[8];
		return str != nullptr &&
			   sscanf(str, "%x:%x:%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3],
					  &v[4], &v[5], &v[6], &v[7]) == 8;
	}

	static void formatUwbAddressFromBytes(const uint8_t bytes[8], char *out,
										  size_t outLen) {
		snprintf(out, outLen,
				 "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X", bytes[0], bytes[1],
				 bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);
	}

	static bool parseUwbByteHex(const char *str, uint8_t *out) {
		if (str == nullptr || str[0] == '\0') {
			return false;
		}
		unsigned int value = 0;
		if (sscanf(str, "%x", &value) != 1 || value > 0xFF) {
			return false;
		}
		*out = (uint8_t)value;
		return true;
	}

	static bool macEquals(const char *macStr, const uint8_t mac[6]) {
		uint8_t parsed[6];
		if (!parseMac(macStr, parsed)) {
			return false;
		}
		return memcmp(parsed, mac, 6) == 0;
	}

	Anchor *searchAnchorByShortAddress(uint16_t anchorAdd) {
		for (int i = 0; i < devices; i++) {
			if (list[i].shortAddress == anchorAdd) {
				return &list[i];
			}
		}
		return nullptr;
	}

	const Anchor *findAnchorByShortAddress(uint16_t anchorAdd) const {
		for (int i = 0; i < devices; i++) {
			if (list[i].shortAddress == anchorAdd) {
				return &list[i];
			}
		}
		return nullptr;
	}

	Anchor *searchAnchorByMac(const uint8_t mac[6]) {
		for (int i = 0; i < devices; i++) {
			if (macEquals(list[i].MAC_Address, mac)) {
				return &list[i];
			}
		}
		return nullptr;
	}

	Anchor *searchAnchorByMacStr(const char *macStr) {
		uint8_t mac[6];
		if (!parseMac(macStr, mac)) {
			return nullptr;
		}
		return searchAnchorByMac(mac);
	}

	void addAnchor(const Anchor &newAnchor) {
		if (devices < EEPROM_ANCHORLIST_SIZE) {
			memcpy(&list[devices], &newAnchor, sizeof(Anchor));
			devices++;
		} else {
			DUMPLN("Max Anchor Devices limit ", EEPROM_ANCHORLIST_SIZE);
		}
	}

	bool upsertAnchor(const Anchor &newAnchor) {
		uint8_t mac[6];
		if (!parseMac(newAnchor.MAC_Address, mac)) {
			DUMPSLN("upsertAnchor: MAC WiFi invalida (se esperan 6 bytes)");
			return false;
		}
		Anchor *existing = searchAnchorByMac(mac);
		if (existing != nullptr) {
			uint16_t prevShort = existing->shortAddress;
			memcpy(existing, &newAnchor, sizeof(Anchor));
			if (newAnchor.shortAddress == 0) {
				existing->shortAddress = prevShort;
			}
			return true;
		}
		if (devices >= EEPROM_ANCHORLIST_SIZE) {
			DUMPSLN("Anchor list full, cannot auto-register");
			return false;
		}
		addAnchor(newAnchor);
		return true;
	}

	/** Inserta o actualiza por short UWB y/o MAC WiFi (registro manual o ESP-NOW). */
	bool upsertAnchorEntry(const Anchor &newAnchor) {
		if (newAnchor.shortAddress != 0) {
			Anchor *byShort =
				searchAnchorByShortAddress(newAnchor.shortAddress);
			if (byShort != nullptr) {
				char prevMac[sizeof(byShort->MAC_Address)];
				strncpy(prevMac, byShort->MAC_Address, sizeof(prevMac));
				prevMac[sizeof(prevMac) - 1] = '\0';
				uint16_t prevShort = byShort->shortAddress;
				memcpy(byShort, &newAnchor, sizeof(Anchor));
				byShort->shortAddress = prevShort;
				if (newAnchor.MAC_Address[0] == '\0' && prevMac[0] != '\0') {
					strncpy(byShort->MAC_Address, prevMac,
							sizeof(byShort->MAC_Address) - 1);
				}
				return true;
			}
		}

		uint8_t mac[6];
		if (newAnchor.MAC_Address[0] != '\0' &&
			parseMac(newAnchor.MAC_Address, mac)) {
			return upsertAnchor(newAnchor);
		}

		if (newAnchor.shortAddress != 0) {
			if (devices >= EEPROM_ANCHORLIST_SIZE) {
				DUMPSLN("Anchor list full, cannot auto-register");
				return false;
			}
			addAnchor(newAnchor);
			return true;
		}

		DUMPSLN("upsertAnchorEntry: hace falta short UWB o MAC WiFi valida");
		return false;
	}

	bool updateShortAddress(const uint8_t mac[6], uint16_t shortAddr) {
		Anchor *found = searchAnchorByMac(mac);
		if (found == nullptr) {
			return false;
		}
		found->shortAddress = shortAddr;
		return true;
	}

	void removeAnchorAt(int anchorNum) {
		DUMPLN("Anchor deleted : ", list[anchorNum].name);
		if (devices > 1) {
			memcpy(&list[anchorNum], &list[devices - 1], sizeof(Anchor));
		}
	}

	bool removeAnchor(const String &anchorName) {
		DUMPLN("Delete anchor Name: ", anchorName);
		if (devices == 0) {
			return false;
		}
		for (int i = 0; i < devices; i++) {
			int n =
				memcmp(list[i].name, anchorName.c_str(), anchorName.length());
			if (n == 0) {
				removeAnchorAt(i);
				devices--;
				return true;
			}
		}
		return false;
	}

	bool removeAnchorByMac(const char *macStr) {
		uint8_t mac[6];
		if (!parseMac(macStr, mac)) {
			return false;
		}
		for (int i = 0; i < devices; i++) {
			if (macEquals(list[i].MAC_Address, mac)) {
				removeAnchorAt(i);
				devices--;
				return true;
			}
		}
		return false;
	}

	bool removeAnchorByShort(uint16_t shortAddr) {
		if (shortAddr == 0) {
			return false;
		}
		for (int i = 0; i < devices; i++) {
			if (list[i].shortAddress == shortAddr) {
				removeAnchorAt(i);
				devices--;
				return true;
			}
		}
		return false;
	}

	void cleanList() { devices = 0; }
};

/** Posición local persistida en el anchor. */
struct AnchorPosition {
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;
	float offset = UWB_DEFAULT_OFFSET_M;
};

#define ESPNOW_MAGIC 0xA5

enum EspNowMsgType : uint8_t {
	MSG_POSITION_SYNC = 1,
	MSG_SYNC_REQUEST = 2,
};

/** Payload ESP-NOW unificado (POD). */
struct EspNowAnchorPacket {
	uint8_t magic;
	uint8_t type;
	uint8_t mac[6];
	char name[10];
	uint16_t shortAddress;
	float x;
	float y;
	float z;
	float offset;
};

#endif
