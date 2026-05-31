#ifndef _INDOOR_UWB_OTA_H
#define _INDOOR_UWB_OTA_H

#include "../config.h"

#if OTA_ENABLED

#include "IndoorUWB_Controller.h"
#include <ArduinoOTA.h>

class IndoorUWB_OTA : public IndoorUWB_Controller {
  public:
	static IndoorUWB_OTA &getInstance() {
		static IndoorUWB_OTA instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_OTA;
	}

	void begin() override {
		ArduinoOTA.setHostname(OTA_HOST);
		ArduinoOTA.onStart([]() { DUMPSLN("OTA start"); });
		ArduinoOTA.onEnd([]() { DUMPSLN("OTA end"); });
		ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
			DUMPF("OTA Progress: %u%%\r", (progress / (total / 100)));
		});
		ArduinoOTA.onError(
			[](ota_error_t error) { DUMPF("OTA Error[%u]\n", error); });
		ArduinoOTA.begin();
		DUMPSLN("OTA ready");
	}

	void update() { ArduinoOTA.handle(); }
};

#endif /* OTA_ENABLED */

#endif
