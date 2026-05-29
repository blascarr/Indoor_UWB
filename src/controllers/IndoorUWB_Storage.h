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
		logPosition();
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
		position.offset = prefs.getFloat(PREFS_KEY_POS_OFFSET, 0.f);
		prefs.end();
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
			(uint16_t)((shortAddr[0] << 8) | shortAddr[1]);
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
	void logPosition() const {
		DUMP("Anchor position X=", position.x);
		DUMP(" Y=", position.y);
		DUMPLN(" Z=", position.z);
	}
#endif
};

#endif
