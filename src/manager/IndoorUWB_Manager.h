#ifndef _INDOOR_UWB_MANAGER_H
#define _INDOOR_UWB_MANAGER_H

#include "../config.h"
#include "../controllers/IndoorUWB_DW1000.h"
#if ESPNOW_ENABLED
#include "../controllers/IndoorUWB_ESPNow.h"
#endif
#include "../controllers/IndoorUWB_OTA.h"
#include "../controllers/IndoorUWB_WebServer.h"
#include "../controllers/IndoorUWB_Wifi.h"
#include "../controllers/IndoorUWB_Storage.h"
#include "types.h"

#if defined(INDOOR_UWB_ROLE_TAG)
#include "../controllers/IndoorUWB_Tag.h"
#endif

class IndoorUWB_Manager {
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

		storage = &IndoorUWB_Storage::getInstance();
		wifi = &IndoorUWB_Wifi::getInstance();
#if ESPNOW_ENABLED
		espNow = &IndoorUWB_ESPNow::getInstance();
#endif
		dw1000 = &IndoorUWB_DW1000::getInstance();
		web = &IndoorUWB_WebServer::getInstance();
		ota = &IndoorUWB_OTA::getInstance();
#if defined(INDOOR_UWB_ROLE_TAG)
		tag = &IndoorUWB_Tag::getInstance();
#endif

		const auto onState = IndoorUWB_Manager::stateChangedCallback;
		wifi->setOnChangeCallback(onState);
		dw1000->setOnChangeCallback(onState);
#if ESPNOW_ENABLED
		espNow->setOnChangeCallback(onState);
#endif
		web->setOnChangeCallback(onState);
		ota->setOnChangeCallback(onState);

		storage->begin();
		wifi->begin();
#if ESPNOW_ENABLED
		espNow->begin();
#endif
		dw1000->begin();
		web->begin();
		ota->begin();
#if defined(INDOOR_UWB_ROLE_TAG)
		tag->begin();
#endif
	}

	void update() {
		wifi->update();
		dw1000->update();
		ota->update();
	}

	static void stateChangedCallback(ControllerType type, status_t prev,
									 status_t current) {
		(void)prev;
		(void)current;
		DUMPLN("Controller state change type=", (int)type);
	}

  private:
	IndoorUWB_Storage *storage = nullptr;
	IndoorUWB_Wifi *wifi = nullptr;
#if ESPNOW_ENABLED
	IndoorUWB_ESPNow *espNow = nullptr;
#endif
	IndoorUWB_DW1000 *dw1000 = nullptr;
	IndoorUWB_WebServer *web = nullptr;
	IndoorUWB_OTA *ota = nullptr;
#if defined(INDOOR_UWB_ROLE_TAG)
	IndoorUWB_Tag *tag = nullptr;
#endif
};

#endif
