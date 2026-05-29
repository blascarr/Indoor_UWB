#ifndef _INDOOR_UWB_STORAGE_H
#define _INDOOR_UWB_STORAGE_H

#include "../config.h"
#include "../models/AnchorModel.h"
#include "IndoorUWB_Controller.h"
#include <Preferences.h>
#include <WiFi.h>

#if defined(INDOOR_UWB_ROLE_ANCHOR)
#include <DW1000Ranging.h>
#endif

class IndoorUWB_Storage : public IndoorUWB_Controller {
  public:
#if defined(INDOOR_UWB_ROLE_TAG)
	AnchorList anchorList;
#endif
#if defined(INDOOR_UWB_ROLE_ANCHOR)
	AnchorPosition position{};
	char uwbAddress[UWB_EUI_STRING_LEN]{};
#endif

	static IndoorUWB_Storage &getInstance() {
		static IndoorUWB_Storage instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_STORAGE;
	}

	void begin() override {
#if defined(INDOOR_UWB_ROLE_TAG)
		loadAnchorList();
		debugAnchorList();
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		loadPosition();
		loadUwbAddress();
		logPosition();
		logUwbAddress();
#endif
	}

#if defined(INDOOR_UWB_ROLE_TAG)
	void saveAnchorList() {
		prefs.begin(PREFS_NAMESPACE, false);
		prefs.putBytes(PREFS_KEY_ANCHOR_LIST, &anchorList, sizeof(AnchorList));
		prefs.end();
	}

	void clearAnchorList() {
		anchorList = AnchorList{};
		saveAnchorList();
		DUMPSLN("Anchor list cleared");
	}

	void loadAnchorList() {
		prefs.begin(PREFS_NAMESPACE, true);
		size_t len = prefs.getBytesLength(PREFS_KEY_ANCHOR_LIST);
		if (len == sizeof(AnchorList)) {
			prefs.getBytes(PREFS_KEY_ANCHOR_LIST, &anchorList, sizeof(AnchorList));
		} else {
			anchorList = AnchorList{};
		}
		prefs.end();
		if (anchorList.fingerprint != ANCHORLIST_FINGERPRINT) {
			DUMPSLN("Invalid anchor list fingerprint, resetting");
			clearAnchorList();
		}
	}

	float lookupOffsetByShort(uint16_t shortAddress) const {
		if (shortAddress == 0) {
			return 0.f;
		}
		const Anchor *found = anchorList.findAnchorByShortAddress(shortAddress);
		if (found != nullptr) {
			return found->o;
		}
		return UWB_DEFAULT_OFFSET_M;
	}

	float correctedRange(uint16_t shortAddress, float rawRange) const {
		return rawRange + lookupOffsetByShort(shortAddress);
	}

	bool updateAnchorOffset(const char *mac, float offset) {
		Anchor *found = anchorList.searchAnchorByMacStr(mac);
		if (found == nullptr) {
			return false;
		}
		found->o = roundMeters(offset, 3);
		saveAnchorList();
		return true;
	}

	bool updateAnchorOffsetByShort(uint16_t shortAddress, float offset) {
		Anchor *found = anchorList.searchAnchorByShortAddress(shortAddress);
		if (found == nullptr) {
			return false;
		}
		found->o = roundMeters(offset, 3);
		saveAnchorList();
		return true;
	}

	bool mergeAnchorByMac(const char *mac, const Anchor &patch) {
		uint8_t macBytes[6];
		if (!AnchorList::parseMac(mac, macBytes)) {
			return false;
		}
		Anchor *found = anchorList.searchAnchorByMac(macBytes);
		if (found == nullptr) {
			return false;
		}
		return applyAnchorPatch(found, patch);
	}

	bool mergeAnchorByShort(uint16_t shortAddress, const Anchor &patch) {
		if (shortAddress == 0) {
			return false;
		}
		Anchor *found = anchorList.searchAnchorByShortAddress(shortAddress);
		if (found == nullptr) {
			return false;
		}
		return applyAnchorPatch(found, patch);
	}

  private:
	bool applyAnchorPatch(Anchor *found, const Anchor &patch) {
		if (patch.name[0] != '\0') {
			strncpy(found->name, patch.name, sizeof(found->name) - 1);
		}
		if (patch.DW1000_Address[0] != '\0') {
			strncpy(found->DW1000_Address, patch.DW1000_Address,
					sizeof(found->DW1000_Address) - 1);
		}
		if (patch.MAC_Address[0] != '\0') {
			strncpy(found->MAC_Address, patch.MAC_Address,
					sizeof(found->MAC_Address) - 1);
		}
		if (patch.shortAddress != 0) {
			found->shortAddress = patch.shortAddress;
		}
		found->x = roundMeters(patch.x, 2);
		found->y = roundMeters(patch.y, 2);
		found->z = roundMeters(patch.z, 2);
		found->o = roundMeters(patch.o, 3);
		saveAnchorList();
		return true;
	}

  public:

	bool applyPositionSync(const EspNowAnchorPacket &pkt) {
		Anchor entry{};
		AnchorList::formatMac(pkt.mac, entry.MAC_Address, sizeof(entry.MAC_Address));
		strncpy(entry.name, pkt.name, sizeof(entry.name) - 1);
		entry.shortAddress = pkt.shortAddress;
		entry.x = pkt.x;
		entry.y = pkt.y;
		entry.z = pkt.z;
		entry.o = pkt.offset;
		if (!anchorList.upsertAnchor(entry)) {
			DUMPSLN("ESP-NOW: lista de anchors llena");
			return false;
		}
		saveAnchorList();
		DUMPF("ESP-NOW: anchor \"%s\" (%s) guardado\n", entry.name,
			  entry.MAC_Address);
		return true;
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	void savePosition() {
		prefs.begin(PREFS_NAMESPACE, false);
		prefs.putFloat(PREFS_KEY_POS_X, position.x);
		prefs.putFloat(PREFS_KEY_POS_Y, position.y);
		prefs.putFloat(PREFS_KEY_POS_Z, position.z);
		prefs.putFloat(PREFS_KEY_POS_OFFSET, position.offset);
		prefs.end();
	}

	void loadPosition() {
		prefs.begin(PREFS_NAMESPACE, true);
		position.x = prefs.getFloat(PREFS_KEY_POS_X, 0.f);
		position.y = prefs.getFloat(PREFS_KEY_POS_Y, 0.f);
		position.z = prefs.getFloat(PREFS_KEY_POS_Z, 0.f);
		position.offset = prefs.getFloat(PREFS_KEY_POS_OFFSET, UWB_DEFAULT_OFFSET_M);
		prefs.end();
	}

	char *getUwbAddress() { return uwbAddress; }

	const char *getUwbAddress() const { return uwbAddress; }

	bool saveUwbAddressFromBytes(const uint8_t bytes[8]) {
		AnchorList::formatUwbAddressFromBytes(bytes, uwbAddress,
											  sizeof(uwbAddress));
		prefs.begin(PREFS_NAMESPACE, false);
		prefs.putString(PREFS_KEY_UWB_ADDRESS, uwbAddress);
		prefs.end();
		DUMPF("UWB address guardada: %s\n", uwbAddress);
		return true;
	}

	bool parseUwbAddressToBytes(uint8_t bytes[8]) const {
		unsigned int values[8];
		if (sscanf(uwbAddress, "%x:%x:%x:%x:%x:%x:%x:%x", &values[0],
				   &values[1], &values[2], &values[3], &values[4], &values[5],
				   &values[6], &values[7]) != 8) {
			return false;
		}
		for (int i = 0; i < 8; i++) {
			bytes[i] = (uint8_t)values[i];
		}
		return true;
	}

	void fillSyncPacket(EspNowAnchorPacket &pkt) const {
		memset(&pkt, 0, sizeof(pkt));
		pkt.magic = ESPNOW_MAGIC;
		pkt.type = MSG_POSITION_SYNC;
		WiFi.macAddress(pkt.mac);
		strncpy(pkt.name, DEVICE_NAME, sizeof(pkt.name) - 1);
		pkt.x = position.x;
		pkt.y = position.y;
		pkt.z = position.z;
		pkt.offset = position.offset;
		byte *shortAddr = DW1000Ranging.getCurrentShortAddress();
		pkt.shortAddress =
			(uint16_t)(shortAddr[1] * 256 + shortAddr[0]);
	}
#endif

  private:
	Preferences prefs;

#if defined(INDOOR_UWB_ROLE_TAG)
	void debugAnchorList() {
		DUMPLN("Anchors in storage: ", anchorList.devices);
		for (int i = 0; i < anchorList.devices; i++) {
			const Anchor &a = anchorList.list[i];
			DUMP("  ", a.name);
			DUMP(" MAC=", a.MAC_Address);
			DUMPLN(" short=", a.shortAddress);
		}
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	void loadUwbAddress() {
		prefs.begin(PREFS_NAMESPACE, true);
		String stored = prefs.getString(PREFS_KEY_UWB_ADDRESS, "");
		prefs.end();
		if (stored.length() > 0 &&
			AnchorList::parseUwbAddress(stored.c_str(), uwbAddress,
										sizeof(uwbAddress))) {
			return;
		}
		AnchorList::parseUwbAddress(ANCHOR_ADDRESS, uwbAddress,
									sizeof(uwbAddress));
	}

	void logPosition() const {
		DUMP("Anchor position X=", position.x);
		DUMP(" Y=", position.y);
		DUMP(" Z=", position.z);
		DUMPLN(" offset=", position.offset);
	}

	void logUwbAddress() const {
		DUMPF("Anchor UWB address: %s\n", uwbAddress);
	}
#endif
};

#endif
