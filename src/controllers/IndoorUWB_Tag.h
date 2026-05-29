#ifndef _INDOOR_UWB_TAG_H
#define _INDOOR_UWB_TAG_H

#include "../config.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_DW1000.h"

#if defined(INDOOR_UWB_ROLE_TAG)

class IndoorUWB_Tag : public IndoorUWB_Controller {
  public:
	static IndoorUWB_Tag &getInstance() {
		static IndoorUWB_Tag instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_TAG;
	}

	void begin() override {
		DUMPSLN("Tag controller ready (UWB loop en DW1000/TickerFree)");
	}
};

#endif

#endif
