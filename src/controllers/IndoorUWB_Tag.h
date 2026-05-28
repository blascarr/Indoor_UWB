#ifndef _INDOOR_UWB_TAG_H
#define _INDOOR_UWB_TAG_H

#include "../config.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_Dw1000.h"

#if defined(INDOOR_UWB_ROLE_TAG)

class IndoorUwb_Tag : public IndoorUwb_Controller {
  public:
	static IndoorUwb_Tag &getInstance() {
		static IndoorUwb_Tag instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_TAG;
	}

	void begin() override {
		setCallback([]() { IndoorUwb_Dw1000::getInstance().loop(); },
					TAG_LOOP_INTERVAL_MS, 0, MILLIS);
		IndoorUwb_Controller::start();
	}
};

#endif

#endif
