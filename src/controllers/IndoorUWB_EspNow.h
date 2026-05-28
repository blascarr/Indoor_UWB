#ifndef _INDOOR_UWB_ESPNOW_H
#define _INDOOR_UWB_ESPNOW_H

#include "../config.h"
#include "../models/AnchorModel.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_Eeprom.h"
#include <WiFi.h>
#include <esp_now.h>

class IndoorUwb_EspNow : public IndoorUwb_Controller {
  public:
	static IndoorUwb_EspNow &getInstance() {
		static IndoorUwb_EspNow instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_ESPNOW;
	}

	void begin() override {
		if (esp_now_init() != ESP_OK) {
			DUMPSLN("Error initializing ESP-NOW");
			return;
		}
		esp_now_register_send_cb(onDataSent);
		esp_now_register_recv_cb(onDataRecv);

		memcpy(peerInfo.peer_addr, ESPNOW_BROADCAST, 6);
		peerInfo.channel = 0;
		peerInfo.encrypt = false;
		if (esp_now_add_peer(&peerInfo) != ESP_OK) {
			DUMPSLN("Failed to add peer");
		} else {
			DUMPSLN("ESP-NOW Initialized");
		}
	}

	static void sendPosition(const EspNowPosition *payload) {
		esp_err_t result = esp_now_send(
			const_cast<uint8_t *>(ESPNOW_BROADCAST),
			reinterpret_cast<const uint8_t *>(payload), sizeof(EspNowPosition));
		if (result == ESP_OK) {
			DUMPSLN("ESP-NOW sent OK");
		} else {
			DUMPSLN("ESP-NOW send error");
		}
	}

  private:
	esp_now_peer_info_t peerInfo{};

	static void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
		(void)mac;
		DUMPLN("ESP-NOW send status: ",
			   status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
	}

	static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
		(void)mac;
#if defined(INDOOR_UWB_ROLE_TAG)
		if (len != (int)sizeof(Anchor)) {
			return;
		}
		Anchor received;
		memcpy(&received, data, sizeof(Anchor));
		DUMPLN("ESP-NOW anchor recv: ", received.name);
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		if (len != (int)sizeof(EspNowPosition)) {
			return;
		}
		EspNowPosition received;
		memcpy(&received, data, sizeof(EspNowPosition));
		IndoorUwb_Eeprom::getInstance().position = received;
		IndoorUwb_Eeprom::getInstance().savePosition(received);
		sendPosition(&received);
#endif
	}
};

#endif
