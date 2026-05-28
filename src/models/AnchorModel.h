#ifndef _INDOOR_UWB_ANCHOR_MODEL_H
#define _INDOOR_UWB_ANCHOR_MODEL_H

#include "../config.h"
#include <Arduino.h>

struct Anchor {
	char name[10];
	char DW1000_Address[20];
	char MAC_Address[16];
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

	Anchor *searchAnchorByShortAddress(uint16_t anchorAdd) {
		for (int i = 0; i < devices; i++) {
			if (list[i].shortAddress == anchorAdd) {
				return &list[i];
			}
		}
		return nullptr;
	}

	void addAnchor(const Anchor &newAnchor) {
		if (devices < EEPROM_ANCHORLIST_SIZE) {
			memcpy(&list[devices], &newAnchor, sizeof(Anchor));
			devices++;
		} else {
			DUMPLN("Max Anchor Devices limit ", EEPROM_ANCHORLIST_SIZE);
		}
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

	void cleanList() { devices = 0; }
};

/** Payload ESP-NOW (POD, sin String). */
struct EspNowPosition {
	uint16_t address;
	float x;
	float y;
	float z;
	float offset;
};

#endif
