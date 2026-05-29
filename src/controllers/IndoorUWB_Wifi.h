#ifndef _INDOOR_UWB_WIFI_H
#define _INDOOR_UWB_WIFI_H

#include "../config.h"
#include "IndoorUWB_Controller.h"
#include <WiFi.h>
#include <functional>

#if defined(INDOOR_UWB_ROLE_TAG) || defined(INDOOR_UWB_ROLE_ANCHOR)
#include "IndoorUWB_ESPNow.h"
#endif

extern "C" {
#include "esp_wifi.h"
}

class IndoorUWB_Wifi : public IndoorUWB_Controller {
  public:
	bool wifiStatus = false;

	static IndoorUWB_Wifi &getInstance() {
		static IndoorUWB_Wifi instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_WIFI;
	}

	void begin() override {
		applyMacAddress();
		wifiConfig();
		IndoorUWB_Controller::start();
		installTicker();
	}

	bool checkWifiConnection() {
		wifiStatus = (WiFi.status() == WL_CONNECTED);
		return wifiStatus;
	}

	void update() { pollTicker(); }

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
		if (!WiFi.config(LOCAL_IP, GATEWAY, SUBNET, PRIMARYDNS, SECONDARYDNS)) {
			DUMPSLN("WiFi: fallo al configurar IP estatica");
		}
		DUMPF("WiFi: conectando a \"%s\" (IP objetivo %s)...\n", WIFI_SSID,
			  LOCAL_IP.toString().c_str());
		WiFi.begin(WIFI_SSID, WIFI_PASS);
	}

	void installTicker() {
#if WIFI_SUPERVISOR_ENABLED
		setCallback(std::bind(&IndoorUWB_Wifi::linkSupervisorTick, this),
					WIFI_SUPERVISOR_POLL_MS);
#else
		setCallback(std::bind(&IndoorUWB_Wifi::checkWifiConnection, this), 500);
#endif
	}

	void logWifiConnected() {
		DUMPF("WiFi conectado — SSID: %s\n", WiFi.SSID().c_str());
		DUMPF("  IP:      %s\n", WiFi.localIP().toString().c_str());
		DUMPF("  Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
		DUMPF("  Mascara: %s\n", WiFi.subnetMask().toString().c_str());
		DUMPF("  RSSI:    %d dBm\n", WiFi.RSSI());
		IndoorUWB_ESPNow::onWifiConnected();
#if defined(INDOOR_UWB_ROLE_TAG)
		if (!_espNowSyncOnConnectDone) {
			_espNowSyncOnConnectDone = true;
			IndoorUWB_ESPNow::requestAnchorSync();
		}
#endif
	}

	void logWifiDisconnected(uint8_t reason) {
		DUMPF("WiFi desconectado (motivo=%u). Reintentando \"%s\"...\n", reason,
			  WIFI_SSID);
	}

#if WIFI_SUPERVISOR_ENABLED
	void linkSupervisorTick() {
		if (WiFi.status() == WL_CONNECTED) {
			if (!wifiStatus) {
				wifiStatus = true;
				logWifiConnected();
			}
			return;
		}
		if (wifiStatus) {
			logWifiDisconnected(WiFi.status());
		}
		wifiStatus = false;
		if (millis() < _nextCycleMs) {
			return;
		}
		DUMPF("WiFi: ciclo de reconexion a \"%s\"...\n", WIFI_SSID);
		for (int i = 0; i < WIFI_CONNECT_ATTEMPTS_PER_CYCLE; i++) {
			if (WiFi.status() == WL_CONNECTED) {
				wifiStatus = true;
				logWifiConnected();
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
		DUMPF("WiFi: sin conexion tras %d intentos. Proximo ciclo en %lu s\n",
			  WIFI_CONNECT_ATTEMPTS_PER_CYCLE, WIFI_RETRY_CYCLE_MS / 1000UL);
		_nextCycleMs = millis() + WIFI_RETRY_CYCLE_MS;
	}
	unsigned long _nextCycleMs = 0;
	bool _espNowSyncOnConnectDone = false;
#endif
};

#endif
