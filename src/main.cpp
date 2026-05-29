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
}
