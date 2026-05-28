#ifndef _INDOOR_UWB_WIFI_H
#define _INDOOR_UWB_WIFI_H

#include "../config.h"
#include "IndoorUWB_Controller.h"
#include <WiFi.h>
#include <functional>

extern "C" {
#include "esp_wifi.h"
}

class IndoorUwb_Wifi : public IndoorUwb_Controller {
  public:
	bool wifiStatus = false;

	static IndoorUwb_Wifi &getInstance() {
		static IndoorUwb_Wifi instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_WIFI;
	}

	void begin() override {
		applyMacAddress();
		wifiConfig();
		IndoorUwb_Controller::start();
		installTicker();
	}

	bool checkWifiConnection() {
		wifiStatus = (WiFi.status() == WL_CONNECTED);
		return wifiStatus;
	}

	void update() {}

  private:
	void applyMacAddress() {
		esp_wifi_set_mac(WIFI_IF_STA, const_cast<uint8_t *>(MACAddress));
		DUMPS("MAC Address: ");
		for (uint8_t i = 0; i < 6; i++) {
			DUMPVHEX(MACAddress[i]);
			DUMPS(":");
		}
		DUMPPRINTLN();
	}

	void wifiConfig() {
		WiFi.mode(WIFI_STA);
		WiFi.begin(WIFI_SSID, WIFI_PASS);
		if (!WiFi.config(LOCAL_IP, GATEWAY, SUBNET, PRIMARYDNS, SECONDARYDNS)) {
			DUMPSLN("STA Failed to configure");
		}
		DUMPSLN("Connecting to WiFi...");
	}

	void installTicker() {
#if WIFI_SUPERVISOR_ENABLED
		setCallback(std::bind(&IndoorUwb_Wifi::linkSupervisorTick, this),
					WIFI_SUPERVISOR_POLL_MS);
#else
		setCallback(std::bind(&IndoorUwb_Wifi::checkWifiConnection, this), 500);
#endif
	}

#if WIFI_SUPERVISOR_ENABLED
	void linkSupervisorTick() {
		if (WiFi.status() == WL_CONNECTED) {
			if (!wifiStatus) {
				wifiStatus = true;
				DUMPLN("WiFi OK - IP: ", WiFi.localIP());
			}
			return;
		}
		wifiStatus = false;
		if (millis() < _nextCycleMs) {
			return;
		}
		for (int i = 0; i < WIFI_CONNECT_ATTEMPTS_PER_CYCLE; i++) {
			if (WiFi.status() == WL_CONNECTED) {
				wifiStatus = true;
				return;
			}
			WiFi.disconnect();
			delay(WIFI_ATTEMPT_GAP_MS);
			WiFi.begin(WIFI_SSID, WIFI_PASS);
			unsigned long deadline =
				millis() + (i == 0 ? WIFI_JOIN_FIRST_ATTEMPT_MS
								   : WIFI_JOIN_TIMEOUT_PER_ATTEMPT_MS);
			while (millis() < deadline && WiFi.status() != WL_CONNECTED) {
				delay(50);
			}
		}
		_nextCycleMs = millis() + WIFI_RETRY_CYCLE_MS;
	}
	unsigned long _nextCycleMs = 0;
#endif
};

#endif
