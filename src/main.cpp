#include <Arduino.h>

#include "config.h"
#include "manager/IndoorUWB_Manager.h"

IndoorUwb_Manager manager;

void setup() {
	Serial.begin(115200);
	manager.begin();
}

void loop() {
	manager.update();
#if defined(INDOOR_UWB_ROLE_ANCHOR)
	IndoorUwb_Dw1000::getInstance().loop();
#endif
}
