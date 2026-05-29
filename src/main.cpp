#include <Arduino.h>

#include "config.h"
#include "manager/IndoorUWB_Manager.h"

IndoorUWB_Manager manager;

void setup() {
	Serial.begin(115200);
	manager.begin();
}

void loop() {
	manager.update();
#if defined(INDOOR_UWB_ROLE_ANCHOR)
	IndoorUWB_DW1000::getInstance().loop();
#endif
}
