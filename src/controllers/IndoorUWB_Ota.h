#ifndef _INDOOR_UWB_OTA_H
#define _INDOOR_UWB_OTA_H

#include "../config.h"
#include "IndoorUWB_Controller.h"
#include <ArduinoOTA.h>

class IndoorUwb_Ota : public IndoorUwb_Controller {
  public:
	static IndoorUwb_Ota &getInstance() {
		static IndoorUwb_Ota instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_OTA;
	}

	void begin() override {
#if OTA_ENABLED
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
#endif
	}

	void update() {
#if OTA_ENABLED
		ArduinoOTA.handle();
#endif
	}
};

#endif
