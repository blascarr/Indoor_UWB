#ifndef _INDOOR_UWB_CONTROLLER_H
#define _INDOOR_UWB_CONTROLLER_H

#include "../manager/types.h"
#include <TickerFree.h>

using StateChangeCallback = void (*)(ControllerType, status_t, status_t);

class IndoorUwb_Controller : public TickerFree<> {
  public:
	StateChangeCallback onChangeCallback = nullptr;
	status_t currentState = STOPPED;
	status_t previousState = STOPPED;

	IndoorUwb_Controller() : TickerFree(nullptr, 1000) {}

	virtual void begin() {}

	void setCallback(CallbackType cb, uint32_t timer = 1000,
					 uint32_t repeat = 0, resolution_t resolution = MICROS) {
		callback = combinedCallback(cb);
		interval(timer);
		repeats(repeat);
	}

	void checkAndUpdateState() {
		currentState = state();
		if (currentState != previousState) {
			if (onChangeCallback) {
				onChangeCallback(getControllerType(), previousState,
								 currentState);
			}
			previousState = currentState;
		}
	}

	void setOnChangeCallback(StateChangeCallback cb) { onChangeCallback = cb; }

	CallbackType combinedCallback(CallbackType cb) {
		return [this, cb]() {
			if (cb) {
				cb();
			}
			checkAndUpdateState();
		};
	}

	virtual ControllerType getControllerType() {
		return ControllerType::CONTROLLER_VOID;
	}
};

#endif
