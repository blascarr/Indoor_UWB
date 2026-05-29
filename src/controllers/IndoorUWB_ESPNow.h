#ifndef _INDOOR_UWB_ESPNOW_H
#define _INDOOR_UWB_ESPNOW_H

#include "../config.h"
#include "../models/AnchorModel.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_Storage.h"
#include <WiFi.h>
#include <esp_now.h>

class IndoorUWB_ESPNow : public IndoorUWB_Controller {
  public:
	static IndoorUWB_ESPNow &getInstance() {
		static IndoorUWB_ESPNow instance;
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

		addPeer(ESPNOW_BROADCAST_MAC);
#if defined(INDOOR_UWB_ROLE_ANCHOR)
		addPeer(ESPNOW_TAG_MAC);
#endif
		DUMPSLN("ESP-NOW initialized");
	}

	static void sendPacket(const EspNowAnchorPacket *payload, bool broadcast) {
#if defined(INDOOR_UWB_ROLE_TAG)
		const uint8_t *dest = ESPNOW_BROADCAST_MAC;
		(void)broadcast;
#else
		const uint8_t *dest =
			broadcast ? ESPNOW_BROADCAST_MAC : ESPNOW_TAG_MAC;
#endif
		esp_err_t result = esp_now_send(
			const_cast<uint8_t *>(dest),
			reinterpret_cast<const uint8_t *>(payload),
			sizeof(EspNowAnchorPacket));
		if (result != ESP_OK) {
			DUMPSLN("ESP-NOW send error");
		}
	}

#if defined(INDOOR_UWB_ROLE_TAG)
	static void requestAnchorSync(uint16_t shortAddress = 0) {
		EspNowAnchorPacket pkt{};
		pkt.magic = ESPNOW_MAGIC;
		pkt.type = MSG_SYNC_REQUEST;
		WiFi.macAddress(pkt.mac);
		pkt.shortAddress = shortAddress;
		sendPacket(&pkt, true);
		DUMPLN("ESP-NOW sync request, short=", shortAddress);
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	static void sendPositionSync() {
		EspNowAnchorPacket pkt{};
		IndoorUWB_Storage::getInstance().fillSyncPacket(pkt);
		sendPacket(&pkt, false);
		DUMPSLN("ESP-NOW position sync sent");
	}
#endif

  private:
	static void addPeer(const uint8_t *mac) {
		esp_now_peer_info_t peerInfo{};
		memcpy(peerInfo.peer_addr, mac, 6);
		peerInfo.channel = 0;
		peerInfo.encrypt = false;
		if (esp_now_is_peer_exist(mac)) {
			return;
		}
		if (esp_now_add_peer(&peerInfo) != ESP_OK) {
			DUMPSLN("Failed to add ESP-NOW peer");
		}
	}

	static void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
		(void)mac;
		DUMPLN("ESP-NOW send status: ",
			   status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
	}

	static void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
		if (len != (int)sizeof(EspNowAnchorPacket)) {
			return;
		}
		EspNowAnchorPacket received;
		memcpy(&received, data, sizeof(EspNowAnchorPacket));
		if (received.magic != ESPNOW_MAGIC) {
			return;
		}

#if defined(INDOOR_UWB_ROLE_TAG)
		if (received.type == MSG_POSITION_SYNC) {
			char macStr[18];
			AnchorList::formatMac(received.mac, macStr, sizeof(macStr));
			DUMPF("ESP-NOW: posicion recibida de %s (%s)\n", received.name,
				  macStr);
			if (IndoorUWB_Storage::getInstance().applyPositionSync(received)) {
				DUMPSLN("ESP-NOW: anchor guardado en NVS");
			}
		}
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		if (received.type == MSG_SYNC_REQUEST) {
			char tagMac[18];
			AnchorList::formatMac(received.mac, tagMac, sizeof(tagMac));
			DUMPF("ESP-NOW: sync solicitado por tag %s (short UWB=0x%04X)\n",
				  tagMac, received.shortAddress);
			delay(random(0, 30));
			EspNowAnchorPacket pkt{};
			IndoorUWB_Storage::getInstance().fillSyncPacket(pkt);
			if (received.shortAddress != 0) {
				pkt.shortAddress = received.shortAddress;
			}
			sendPacket(&pkt, false);
			DUMPF("ESP-NOW: posicion enviada al tag (%s, %.2f, %.2f, %.2f)\n",
				  pkt.name, pkt.x, pkt.y, pkt.z);
		}
#endif
		(void)mac;
	}
};

#endif
