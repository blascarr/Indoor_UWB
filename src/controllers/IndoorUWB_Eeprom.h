#ifndef _INDOOR_UWB_EEPROM_H
#define _INDOOR_UWB_EEPROM_H

#include "../config.h"
#include "../models/AnchorModel.h"
#include "IndoorUWB_Controller.h"
#include <EEPROM.h>

class IndoorUwb_Eeprom : public IndoorUwb_Controller {
  public:
#if defined(INDOOR_UWB_ROLE_TAG)
	AnchorList anchorList;
#endif
#if defined(INDOOR_UWB_ROLE_ANCHOR)
	EspNowPosition position{};
#endif

	static IndoorUwb_Eeprom &getInstance() {
		static IndoorUwb_Eeprom instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_EEPROM;
	}

	void begin() override {
#if defined(INDOOR_UWB_ROLE_TAG)
		EEPROM.begin(sizeof(AnchorList));
		anchorList = loadAnchorList();
		debugAnchorList(anchorList);
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		EEPROM.begin(sizeof(EspNowPosition));
		position = loadPosition();
		logPosition(position);
#endif
	}

#if defined(INDOOR_UWB_ROLE_TAG)
	void saveAnchorList(const AnchorList &list) {
		EEPROM.put(0, list);
		EEPROM.commit();
	}

	void clearAnchorList(AnchorList &list) {
		AnchorList empty;
		saveAnchorList(empty);
		list.cleanList();
		DUMPSLN("Anchor List initialized");
	}

	AnchorList loadAnchorList() {
		AnchorList data;
		EEPROM.get(0, data);
		return data;
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	void savePosition(const EspNowPosition &data) {
		EEPROM.put(0, data);
		EEPROM.commit();
	}

	EspNowPosition loadPosition() {
		EspNowPosition data;
		EEPROM.get(0, data);
		return data;
	}
#endif

  private:
#if defined(INDOOR_UWB_ROLE_TAG)
	void debugAnchorList(AnchorList &list) {
		if (list.fingerprint != ANCHORLIST_FINGERPRINT) {
			DUMPSLN("Fail fingerprint, anchor list not initialized");
			clearAnchorList(list);
			return;
		}
		DUMPLN("Devices in List ", list.devices);
		for (int i = 0; i < list.devices; i++) {
			const Anchor &a = list.list[i];
			DUMP("Name: ", a.name);
			DUMP(" DW1000: ", a.DW1000_Address);
			DUMP(" MAC: ", a.MAC_Address);
			DUMPLN(" short: ", a.shortAddress);
		}
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	void logPosition(const EspNowPosition &p) {
		DUMP("Address: ", p.address);
		DUMP(" X: ", p.x);
		DUMP(" Y: ", p.y);
		DUMPLN(" Z: ", p.z);
	}
#endif
};

#endif
