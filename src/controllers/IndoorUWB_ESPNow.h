#ifndef _INDOOR_UWB_ESPNOW_H
#define _INDOOR_UWB_ESPNOW_H

#include "../config.h"

#if ESPNOW_ENABLED

#include "../models/AnchorModel.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_Storage.h"
#include <WiFi.h>
#include <esp_now.h>

#if defined(INDOOR_UWB_ROLE_ANCHOR)
#include <DW1000Ranging.h>
#endif

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
		_wifiPeersReady = (WiFi.status() == WL_CONNECTED);
		DUMPSLN("ESP-NOW initialized");
	}

	/** Tras conectar WiFi: fija canal ESP-NOW al del AP. */
	static void onWifiConnected() {
		if (WiFi.status() != WL_CONNECTED) {
			return;
		}
		addPeer(ESPNOW_BROADCAST_MAC);
#if defined(INDOOR_UWB_ROLE_ANCHOR)
		addPeer(ESPNOW_TAG_MAC);
		if (_lastTagMacValid) {
			addPeer(_lastTagMac);
		}
#endif
		_wifiPeersReady = true;
		DUMPF("ESP-NOW: peers en canal WiFi %u\n", WiFi.channel());
	}

	static bool isReady() {
		return _wifiPeersReady && WiFi.status() == WL_CONNECTED;
	}

#if defined(INDOOR_UWB_ROLE_TAG)
	static bool requestAnchorSync(uint16_t shortAddress = 0) {
		if (WiFi.status() != WL_CONNECTED) {
			DUMPSLN("ESP-NOW sync: WiFi no conectado");
			return false;
		}
		onWifiConnected();
		EspNowAnchorPacket pkt{};
		pkt.magic = ESPNOW_MAGIC;
		pkt.type = MSG_SYNC_REQUEST;
		WiFi.macAddress(pkt.mac);
		pkt.shortAddress = shortAddress;
		sendPacketBroadcast(&pkt);
		DUMPLN("ESP-NOW sync request, short=", shortAddress);
		return true;
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	static void sendPositionSync() {
		if (WiFi.status() != WL_CONNECTED) {
			DUMPSLN("ESP-NOW sync: WiFi no conectado");
			return;
		}
		onWifiConnected();
		EspNowAnchorPacket pkt{};
		IndoorUWB_Storage::getInstance().fillSyncPacket(pkt);
		if (_lastTagMacValid) {
			sendPacketTo(_lastTagMac, &pkt);
		} else {
			sendPacketTo(ESPNOW_TAG_MAC, &pkt);
		}
		DUMPSLN("ESP-NOW position sync sent");
	}
#endif

  private:
	static bool _wifiPeersReady;
#if defined(INDOOR_UWB_ROLE_ANCHOR)
	static bool _lastTagMacValid;
	static uint8_t _lastTagMac[6];
#endif

	static uint8_t peerChannel() {
		return WiFi.status() == WL_CONNECTED ? WiFi.channel() : 0;
	}

	static void addPeer(const uint8_t *mac) {
		if (mac == nullptr) {
			return;
		}
		esp_now_peer_info_t peerInfo{};
		memcpy(peerInfo.peer_addr, mac, 6);
		peerInfo.channel = peerChannel();
		peerInfo.encrypt = false;
		if (esp_now_is_peer_exist(mac)) {
			return;
		}
		if (esp_now_add_peer(&peerInfo) != ESP_OK) {
			DUMPSLN("Failed to add ESP-NOW peer");
		}
	}

	static void sendPacketTo(const uint8_t *dest,
							 const EspNowAnchorPacket *payload) {
		if (dest == nullptr || payload == nullptr) {
			return;
		}
		addPeer(dest);
		esp_err_t result = esp_now_send(
			const_cast<uint8_t *>(dest),
			reinterpret_cast<const uint8_t *>(payload),
			sizeof(EspNowAnchorPacket));
		if (result != ESP_OK) {
			DUMPSLN("ESP-NOW send error");
		}
	}

	static void sendPacketBroadcast(const EspNowAnchorPacket *payload) {
		sendPacketTo(ESPNOW_BROADCAST_MAC, payload);
	}

	static void onDataSent(const esp_now_send_info_t *txInfo,
						   esp_now_send_status_t status) {
		(void)txInfo;
		DUMPLN("ESP-NOW send status: ",
			   status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
	}

	static void onDataRecv(const esp_now_recv_info_t *recvInfo,
						   const uint8_t *data, int len) {
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
			addPeer(received.mac);
			char macStr[18];
			AnchorList::formatMac(received.mac, macStr, sizeof(macStr));
			DUMPF("ESP-NOW: posicion recibida de %s (%s) short=0x%04X\n",
				  received.name, macStr, received.shortAddress);
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

			const uint16_t myShort = DW1000Ranging.getCurrentShortAddress()[1] *
									 256 +
									 DW1000Ranging.getCurrentShortAddress()[0];
			if (received.shortAddress != 0 &&
				received.shortAddress != myShort) {
				return;
			}

			memcpy(_lastTagMac, received.mac, 6);
			_lastTagMacValid = true;
			addPeer(received.mac);

			delay(random(5, 40));
			EspNowAnchorPacket pkt{};
			IndoorUWB_Storage::getInstance().fillSyncPacket(pkt);
			if (received.shortAddress != 0) {
				pkt.shortAddress = received.shortAddress;
			}

			/* Respuesta broadcast: el tag ya tiene peer broadcast y puede recibir
			 * aunque aún no conozca la MAC WiFi del anchor. */
			sendPacketBroadcast(&pkt);
			DUMPF("ESP-NOW: posicion enviada (%s, short=0x%04X, %.2f, %.2f, "
				  "%.2f, offset %+.3f)\n",
				  pkt.name, pkt.shortAddress, pkt.x, pkt.y, pkt.z, pkt.offset);
		}
#endif
		(void)recvInfo;
	}
};

bool IndoorUWB_ESPNow::_wifiPeersReady = false;
#if defined(INDOOR_UWB_ROLE_ANCHOR)
bool IndoorUWB_ESPNow::_lastTagMacValid = false;
uint8_t IndoorUWB_ESPNow::_lastTagMac[6] = {0};
#endif

#endif /* ESPNOW_ENABLED */

#endif
