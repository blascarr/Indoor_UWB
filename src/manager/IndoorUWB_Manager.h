#ifndef _INDOOR_UWB_MANAGER_H
#define _INDOOR_UWB_MANAGER_H

#include "../config.h"
#include "../controllers/IndoorUWB_Dw1000.h"
#include "../controllers/IndoorUWB_Eeprom.h"
#include "../controllers/IndoorUWB_EspNow.h"
#include "../controllers/IndoorUWB_Ota.h"
#include "../controllers/IndoorUWB_WebServer.h"
#include "../controllers/IndoorUWB_Wifi.h"
#include "types.h"

#if defined(INDOOR_UWB_ROLE_TAG)
#include "../controllers/IndoorUWB_Tag.h"
#endif

class IndoorUwb_Manager {
  public:
	void begin() {
		delay(500);
		DUMPSLN("=== Indoor_UWB ===");
		DUMPLN("Role: ",
#if defined(INDOOR_UWB_ROLE_TAG)
			   "TAG"
#else
			   "ANCHOR"
#endif
		);

		eeprom = &IndoorUwb_Eeprom::getInstance();
		wifi = &IndoorUwb_Wifi::getInstance();
		espNow = &IndoorUwb_EspNow::getInstance();
		dw1000 = &IndoorUwb_Dw1000::getInstance();
		web = &IndoorUwb_WebServer::getInstance();
		ota = &IndoorUwb_Ota::getInstance();
#if defined(INDOOR_UWB_ROLE_TAG)
		tag = &IndoorUwb_Tag::getInstance();
#endif

		const auto onState = IndoorUwb_Manager::stateChangedCallback;
		wifi->setOnChangeCallback(onState);
		dw1000->setOnChangeCallback(onState);
		espNow->setOnChangeCallback(onState);
		web->setOnChangeCallback(onState);
		ota->setOnChangeCallback(onState);

		eeprom->begin();
		wifi->begin();
		espNow->begin();
		dw1000->begin();
		web->begin();
		ota->begin();
#if defined(INDOOR_UWB_ROLE_TAG)
		tag->begin();
#endif
	}

	void update() {
		wifi->update();
		ota->update();
#if defined(INDOOR_UWB_ROLE_ANCHOR)
		dw1000->loop();
#endif
	}

	static void stateChangedCallback(ControllerType type, status_t prev,
									 status_t current) {
		(void)prev;
		(void)current;
		DUMPLN("Controller state change type=", (int)type);
	}

  private:
	IndoorUwb_Eeprom *eeprom = nullptr;
	IndoorUwb_Wifi *wifi = nullptr;
	IndoorUwb_EspNow *espNow = nullptr;
	IndoorUwb_Dw1000 *dw1000 = nullptr;
	IndoorUwb_WebServer *web = nullptr;
	IndoorUwb_Ota *ota = nullptr;
#if defined(INDOOR_UWB_ROLE_TAG)
	IndoorUwb_Tag *tag = nullptr;
#endif
};

#endif
