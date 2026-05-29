#ifndef _INDOOR_UWB_WEBSERVER_H
#define _INDOOR_UWB_WEBSERVER_H

#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "../config.h"
#include "../models/AnchorModel.h"
#include "../models/TagPosition.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_DW1000.h"
#include "IndoorUWB_ESPNow.h"
#include "IndoorUWB_Storage.h"
#include "IndoorUWB_Wifi.h"
#include <DW1000Ranging.h>

class IndoorUWB_WebServer : public IndoorUWB_Controller {
  public:
	AsyncWebServer server{SERVER_PORT};

	static IndoorUWB_WebServer &getInstance() {
		static IndoorUWB_WebServer instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_WEB;
	}

	void begin() override {
		if (!LittleFS.begin(true)) {
			DUMPSLN("LittleFS mount failed");
		} else {
			DUMPSLN("LittleFS mounted");
			if (!LittleFS.exists("/index.html")) {
				DUMPSLN(
					"AVISO: falta index.html — sube el FS: pio run -e uwb_tag -t "
					"uploadfs");
			}
		}

#if defined(INDOOR_UWB_ROLE_TAG)
		registerTagRoutes();
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		registerAnchorRoutes();
#endif

		server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
		server.onNotFound([](AsyncWebServerRequest *request) {
			if (request->method() == HTTP_OPTIONS) {
				request->send(204);
				return;
			}
			request->send(404, "text/plain", "Not found");
		});

		server.begin();
		DUMPSLN("Async web server started");
	}

  private:
	static void addCors(AsyncWebServerResponse *response) {
		response->addHeader("Access-Control-Allow-Origin", "*");
		response->addHeader("Access-Control-Allow-Methods",
							"GET, POST, DELETE, OPTIONS");
		response->addHeader("Access-Control-Allow-Headers", "Content-Type");
	}

#if defined(INDOOR_UWB_ROLE_TAG)
	static float lookupLiveRange(uint16_t shortAddress, float *rawOut = nullptr) {
		const uint8_t n = DW1000Ranging.getNetworkDevicesNumber();
		for (uint8_t i = 0; i < n; i++) {
			DW1000Device *dev = DW1000Ranging.getNetworkDeviceAt(i);
			if (dev && dev->getShortAddress() == shortAddress) {
				const float raw = dev->getRange();
				if (rawOut != nullptr) {
					*rawOut = raw;
				}
				return IndoorUWB_Storage::getInstance().correctedRange(
					shortAddress, raw);
			}
		}
		return -1.f;
	}

	String anchorsToJson() {
		AnchorList &list = IndoorUWB_Storage::getInstance().anchorList;
		JSONVar configured;
		for (int i = 0; i < list.devices; i++) {
			const Anchor &a = list.list[i];
			configured[i]["name"] = String(a.name);
			configured[i]["mac"] = String(a.MAC_Address);
			if (a.DW1000_Address[0] != '\0') {
				configured[i]["uwb_address"] = String(a.DW1000_Address);
			}
			configured[i]["shortAddress"] = a.shortAddress;
			configured[i]["x"] = roundMeters(a.x, 2);
			configured[i]["y"] = roundMeters(a.y, 2);
			configured[i]["z"] = roundMeters(a.z, 2);
			configured[i]["offset"] = roundMeters(a.o, 3);
			if (a.shortAddress != 0) {
				float rawRange = -1.f;
				float liveRange =
					lookupLiveRange(a.shortAddress, &rawRange);
				if (liveRange >= 0.f) {
					configured[i]["liveRange"] = liveRange;
					configured[i]["rawRange"] = rawRange;
				}
			}
		}

		JSONVar uwb;
		const uint8_t n = DW1000Ranging.getNetworkDevicesNumber();
		for (uint8_t i = 0; i < n; i++) {
			DW1000Device *dev = DW1000Ranging.getNetworkDeviceAt(i);
			if (!dev) {
				continue;
			}
			uwb[i]["shortAddress"] = dev->getShortAddress();
			const float raw = dev->getRange();
			uwb[i]["range"] = raw;
			uwb[i]["rawRange"] = raw;
			const float corrected =
				IndoorUWB_Storage::getInstance().correctedRange(
					dev->getShortAddress(), raw);
			uwb[i]["correctedRange"] = corrected;
			uwb[i]["offset"] = IndoorUWB_Storage::getInstance().lookupOffsetByShort(
				dev->getShortAddress());
			uwb[i]["rxPower"] = dev->getRXPower();
			uwb[i]["active"] = !dev->isInactive();
		}

		JSONVar root;
		root["configured"] = configured;
		root["uwb"] = uwb;
		root["uwb_count"] = n;
		return JSON.stringify(root);
	}

	String mapToJson() {
		AnchorList &list = IndoorUWB_Storage::getInstance().anchorList;
		JSONVar anchors;
		float minX = 0.f, maxX = 0.f, minY = 0.f, maxY = 0.f;
		bool boundsInit = false;

		for (int i = 0; i < list.devices; i++) {
			const Anchor &a = list.list[i];
			anchors[i]["name"] = String(a.name);
			anchors[i]["shortAddress"] = a.shortAddress;
			anchors[i]["x"] = roundMeters(a.x, 2);
			anchors[i]["y"] = roundMeters(a.y, 2);
			anchors[i]["z"] = roundMeters(a.z, 2);
			anchors[i]["offset"] = roundMeters(a.o, 3);
			if (a.MAC_Address[0] != '\0') {
				anchors[i]["mac"] = String(a.MAC_Address);
			}
			const bool atOrigin =
				fabs(a.x) < 0.01f && fabs(a.y) < 0.01f && fabs(a.z) < 0.01f;
			anchors[i]["isOrigin"] = atOrigin;

			float rawRange = -1.f;
			const float liveRange =
				a.shortAddress != 0
					? lookupLiveRange(a.shortAddress, &rawRange)
					: -1.f;
			anchors[i]["active"] = liveRange >= 0.f;
			if (liveRange >= 0.f) {
				anchors[i]["liveRange"] = liveRange;
				anchors[i]["rawRange"] = rawRange;
			}

			if (!boundsInit) {
				minX = maxX = a.x;
				minY = maxY = a.y;
				boundsInit = true;
			} else {
				if (a.x < minX) {
					minX = a.x;
				}
				if (a.x > maxX) {
					maxX = a.x;
				}
				if (a.y < minY) {
					minY = a.y;
				}
				if (a.y > maxY) {
					maxY = a.y;
				}
			}
		}

		if (gTagPosition.valid) {
			const float px = gTagPosition.pos(0, 0);
			const float py = gTagPosition.pos(1, 0);
			if (!boundsInit) {
				minX = maxX = px;
				minY = maxY = py;
				boundsInit = true;
			} else {
				if (px < minX) {
					minX = px;
				}
				if (px > maxX) {
					maxX = px;
				}
				if (py < minY) {
					minY = py;
				}
				if (py > maxY) {
					maxY = py;
				}
			}
		}

		JSONVar origin;
		origin["x"] = 0;
		origin["y"] = 0;
		origin["z"] = 0;
		origin["label"] = "Origen";

		JSONVar bounds;
		if (boundsInit) {
			const float pad = 1.f;
			bounds["minX"] = roundMeters(minX - pad, 2);
			bounds["maxX"] = roundMeters(maxX + pad, 2);
			bounds["minY"] = roundMeters(minY - pad, 2);
			bounds["maxY"] = roundMeters(maxY + pad, 2);
		}

		JSONVar position;
		position["valid"] = gTagPosition.valid;
		position["anchorsUsed"] = gTagPosition.anchorsUsed;
		if (gTagPosition.valid) {
			position["x"] = roundMeters(gTagPosition.pos(0, 0), 2);
			position["y"] = roundMeters(gTagPosition.pos(1, 0), 2);
			position["z"] = roundMeters(gTagPosition.pos(2, 0), 2);
			position["residual"] = roundMeters(gTagPosition.residual, 3);
		}
		position["updatedMs"] = (double)gTagPosition.updatedMs;

		JSONVar root;
		root["trilateration_mode"] = trilaterationModeStr();
		root["origin"] = origin;
		if (boundsInit) {
			root["bounds"] = bounds;
		}
		root["anchors"] = anchors;
		root["anchor_count"] = list.devices;
		root["uwb_count"] = DW1000Ranging.getNetworkDevicesNumber();
		root["position"] = position;
		return JSON.stringify(root);
	}

	String debugToJson() {
		JSONVar info;
		info["role"] = "tag";
		info["wifi_connected"] = IndoorUWB_Wifi::getInstance().wifiStatus;
		info["wifi_status"] = (int)WiFi.status();
		info["wifi_ssid"] = WiFi.SSID();
		info["ip"] = WiFi.localIP().toString();
		info["uwb_network_devices"] = DW1000Ranging.getNetworkDevicesNumber();
		info["anchors_nvs"] =
			IndoorUWB_Storage::getInstance().anchorList.devices;
		info["uwb_debug"] = UWB_DEBUG;
		info["print_debug"] = PRINTDEBUG;

		JSONVar devices;
		const uint8_t n = DW1000Ranging.getNetworkDevicesNumber();
		for (uint8_t i = 0; i < n; i++) {
			DW1000Device *dev = DW1000Ranging.getNetworkDeviceAt(i);
			if (!dev) {
				continue;
			}
			devices[i]["shortAddress"] = dev->getShortAddress();
			devices[i]["range_m"] = dev->getRange();
			devices[i]["rxPower_dbm"] = dev->getRXPower();
			devices[i]["active"] = !dev->isInactive();
		}
		info["uwb_devices"] = devices;

		if (n == 0) {
			info["hint"] =
				"Sin dispositivos UWB en ranging. Comprueba anchor encendido, "
				"mismo protocolo DW1000Ranging y alcance radio.";
		}
		return JSON.stringify(info);
	}

	void registerTagRoutes() {
		server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
			JSONVar info;
			info["role"] = "tag";
			info["mac"] = WiFi.macAddress();
			info["ip"] = WiFi.localIP().toString();
			info["wifi_connected"] = IndoorUWB_Wifi::getInstance().wifiStatus;
			info["anchor_count"] =
				IndoorUWB_Storage::getInstance().anchorList.devices;
			info["uwb_count"] = DW1000Ranging.getNetworkDevicesNumber();
			info["trilateration_mode"] = trilaterationModeStr();
			AsyncWebServerResponse *response =
				request->beginResponse(200, "application/json",
									   JSON.stringify(info));
			addCors(response);
			request->send(response);
		});

		server.on("/api/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse(
				200, "application/json",
				IndoorUWB_WebServer::getInstance().debugToJson());
			addCors(response);
			request->send(response);
		});

		server.on("/api/anchors", HTTP_GET, [](AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse(
				200, "application/json",
				IndoorUWB_WebServer::getInstance().anchorsToJson());
			addCors(response);
			request->send(response);
		});

		server.on("/api/map", HTTP_GET, [](AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse(
				200, "application/json",
				IndoorUWB_WebServer::getInstance().mapToJson());
			addCors(response);
			request->send(response);
		});

		server.on(
			"/api/anchors", HTTP_POST,
			[](AsyncWebServerRequest *request) {
				if (request->contentLength() == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
				}
			},
			nullptr,
			[](AsyncWebServerRequest *request, uint8_t *data, size_t len,
			   size_t index, size_t total) {
				if (index + len < total) {
					return;
				}
				DUMPF("Web POST /api/anchors (%u bytes)\n", (unsigned)total);
				if (total == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
					return;
				}
				JSONVar body =
					JSON.parse(String((const char *)data, (unsigned)total));
				if (JSON.typeof(body) == "undefined") {
					request->send(400, "application/json",
								  "{\"error\":\"invalid json\"}");
					return;
				}

				const bool hasMacField = body.hasOwnProperty("wifi_mac") ||
										 body.hasOwnProperty("mac");
				const bool hasShortField =
					body.hasOwnProperty("shortAddress") &&
					(int)body["shortAddress"] != 0;
				if (!hasMacField && !hasShortField) {
					DUMPSLN("Web POST /api/anchors: short o wifi_mac");
					request->send(
						400, "application/json",
						"{\"error\":\"shortAddress or wifi_mac required\","
						"\"hint\":\"Registro manual: short UWB + posicion. "
						"MAC WiFi opcional (se obtiene con ESP-NOW).\"}");
					return;
				}

				Anchor entry{};
				String macField;
				if (body.hasOwnProperty("wifi_mac")) {
					macField = (const char *)body["wifi_mac"];
				} else if (body.hasOwnProperty("mac")) {
					macField = (const char *)body["mac"];
				}
				macField.toUpperCase();
				macField.trim();

				String uwbField;
				if (body.hasOwnProperty("uwb_address")) {
					uwbField = (const char *)body["uwb_address"];
					uwbField.toUpperCase();
					uwbField.trim();
				}

				if (macField.length() > 0 &&
					AnchorList::isUwbAddressFormat(macField.c_str())) {
					if (uwbField.length() == 0) {
						uwbField = macField;
					}
					macField = "";
				}

				if (macField.length() > 0) {
					uint8_t macBytes[6];
					if (!AnchorList::parseMac(macField.c_str(), macBytes)) {
						request->send(
							400, "application/json",
							"{\"error\":\"invalid_wifi_mac\","
							"\"hint\":\"Formato esperado: AA:BB:CC:DD:EE:FF "
							"(6 bytes WiFi)\"}");
						return;
					}
					AnchorList::formatMac(macBytes, entry.MAC_Address,
										  sizeof(entry.MAC_Address));
				}

				if (uwbField.length() > 0) {
					AnchorList::parseUwbAddress(
						uwbField.c_str(), entry.DW1000_Address,
						sizeof(entry.DW1000_Address));
				}
				if (body.hasOwnProperty("shortAddress")) {
					entry.shortAddress = (uint16_t)(int)body["shortAddress"];
				}
				if (body.hasOwnProperty("name")) {
					strncpy(entry.name, (const char *)body["name"],
							sizeof(entry.name) - 1);
				}
				if (body.hasOwnProperty("x")) {
					entry.x = roundMeters((float)(double)body["x"], 2);
				}
				if (body.hasOwnProperty("y")) {
					entry.y = roundMeters((float)(double)body["y"], 2);
				}
				if (body.hasOwnProperty("z")) {
					entry.z = roundMeters((float)(double)body["z"], 2);
				}
				if (body.hasOwnProperty("offset")) {
					entry.o = roundMeters((float)(double)body["offset"], 3);
				} else {
					entry.o = UWB_DEFAULT_OFFSET_M;
				}

				if (entry.shortAddress == 0 && entry.MAC_Address[0] == '\0') {
					request->send(
						400, "application/json",
						"{\"error\":\"shortAddress or wifi_mac required\"}");
					return;
				}

				AnchorList &list =
					IndoorUWB_Storage::getInstance().anchorList;
				if (!list.upsertAnchorEntry(entry)) {
					DUMPSLN("Web POST /api/anchors: lista llena");
					request->send(507, "application/json",
								  "{\"error\":\"anchor list full\"}");
					return;
				}
				IndoorUWB_Storage::getInstance().saveAnchorList();
				DUMPF("Web: anchor guardado \"%s\" MAC %s short=0x%04X\n",
					  entry.name, entry.MAC_Address, entry.shortAddress);
				JSONVar resp;
				resp["ok"] = true;
				resp["mac"] = String(entry.MAC_Address);
				resp["shortAddress"] = entry.shortAddress;
				resp["name"] = String(entry.name);
				request->send(200, "application/json", JSON.stringify(resp));
			});

		server.on("/api/anchors", HTTP_DELETE,
				  [](AsyncWebServerRequest *request) {
					  AnchorList &list =
						  IndoorUWB_Storage::getInstance().anchorList;
					  if (request->hasParam("short")) {
						  const uint16_t shortAddr = (uint16_t)request
														 ->getParam("short")
														 ->value()
														 .toInt();
						  if (shortAddr == 0 ||
							  !list.removeAnchorByShort(shortAddr)) {
							  request->send(404, "application/json",
											"{\"error\":\"not found\"}");
							  return;
						  }
						  IndoorUWB_Storage::getInstance().saveAnchorList();
						  DUMPF("Web: anchor eliminado short=0x%04X\n",
								shortAddr);
						  request->send(200, "application/json",
										"{\"ok\":true}");
						  return;
					  }
					  if (!request->hasParam("mac")) {
						  request->send(400, "application/json",
										"{\"error\":\"mac or short required\"}");
						  return;
					  }
					  String mac = request->getParam("mac")->value();
					  mac.toUpperCase();
					  if (!list.removeAnchorByMac(mac.c_str())) {
						  request->send(404, "application/json",
										"{\"error\":\"not found\"}");
						  return;
					  }
					  IndoorUWB_Storage::getInstance().saveAnchorList();
					  DUMPF("Web: anchor eliminado MAC %s\n", mac.c_str());
					  request->send(200, "application/json", "{\"ok\":true}");
				  });

		server.on(
			"/api/anchors/offset", HTTP_POST,
			[](AsyncWebServerRequest *request) {
				if (request->contentLength() == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
				}
			},
			nullptr,
			[](AsyncWebServerRequest *request, uint8_t *data, size_t len,
			   size_t index, size_t total) {
				if (index + len < total) {
					return;
				}
				if (total == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
					return;
				}
				JSONVar body =
					JSON.parse(String((const char *)data, (unsigned)total));
				if (JSON.typeof(body) == "undefined" ||
					!body.hasOwnProperty("offset")) {
					request->send(400, "application/json",
								  "{\"error\":\"offset required\"}");
					return;
				}
				const float offset =
					roundMeters((float)(double)body["offset"], 3);
				IndoorUWB_Storage &storage =
					IndoorUWB_Storage::getInstance();
				bool ok = false;
				if (body.hasOwnProperty("mac")) {
					String mac = (const char *)body["mac"];
					mac.toUpperCase();
					mac.trim();
					ok = storage.updateAnchorOffset(mac.c_str(), offset);
				} else if (body.hasOwnProperty("shortAddress")) {
					const uint16_t shortAddr =
						(uint16_t)(int)body["shortAddress"];
					ok = storage.updateAnchorOffsetByShort(shortAddr, offset);
				} else {
					request->send(400, "application/json",
								  "{\"error\":\"mac or shortAddress required\"}");
					return;
				}
				if (!ok) {
					request->send(404, "application/json",
								  "{\"error\":\"anchor not found in NVS\"}");
					return;
				}
				DUMPF("Web: offset actualizado = %+.3f m\n", offset);
				JSONVar resp;
				resp["ok"] = true;
				resp["offset"] = offset;
				request->send(200, "application/json", JSON.stringify(resp));
			});

		server.on(
			"/api/anchors", HTTP_PUT,
			[](AsyncWebServerRequest *request) {
				if (request->contentLength() == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
				}
			},
			nullptr,
			[](AsyncWebServerRequest *request, uint8_t *data, size_t len,
			   size_t index, size_t total) {
				if (index + len < total) {
					return;
				}
				if (total == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
					return;
				}
				JSONVar body =
					JSON.parse(String((const char *)data, (unsigned)total));
				if (JSON.typeof(body) == "undefined") {
					request->send(400, "application/json",
								  "{\"error\":\"invalid json\"}");
					return;
				}

				String mac;
				if (body.hasOwnProperty("mac")) {
					mac = (const char *)body["mac"];
					mac.toUpperCase();
					mac.trim();
				}
				uint16_t lookupShort = 0;
				if (body.hasOwnProperty("lookupShortAddress")) {
					lookupShort = (uint16_t)(int)body["lookupShortAddress"];
				}
				if (mac.length() == 0 && lookupShort == 0) {
					request->send(
						400, "application/json",
						"{\"error\":\"mac or lookupShortAddress required\"}");
					return;
				}

				AnchorList &list =
					IndoorUWB_Storage::getInstance().anchorList;
				Anchor *existing = nullptr;
				if (mac.length() > 0) {
					existing = list.searchAnchorByMacStr(mac.c_str());
				}
				if (existing == nullptr && lookupShort != 0) {
					existing = list.searchAnchorByShortAddress(lookupShort);
				}
				if (existing == nullptr) {
					request->send(404, "application/json",
								  "{\"error\":\"anchor not found\"}");
					return;
				}

				Anchor patch = *existing;
				if (body.hasOwnProperty("name")) {
					strncpy(patch.name, (const char *)body["name"],
							sizeof(patch.name) - 1);
				}
				if (body.hasOwnProperty("uwb_address")) {
					String uwb = (const char *)body["uwb_address"];
					uwb.toUpperCase();
					AnchorList::parseUwbAddress(uwb.c_str(), patch.DW1000_Address,
												sizeof(patch.DW1000_Address));
				}
				if (body.hasOwnProperty("shortAddress")) {
					patch.shortAddress = (uint16_t)(int)body["shortAddress"];
				}
				if (body.hasOwnProperty("x")) {
					patch.x = roundMeters((float)(double)body["x"], 2);
				}
				if (body.hasOwnProperty("y")) {
					patch.y = roundMeters((float)(double)body["y"], 2);
				}
				if (body.hasOwnProperty("z")) {
					patch.z = roundMeters((float)(double)body["z"], 2);
				}
				if (body.hasOwnProperty("offset")) {
					patch.o = roundMeters((float)(double)body["offset"], 3);
				}

				IndoorUWB_Storage &storage =
					IndoorUWB_Storage::getInstance();
				bool ok = false;
				if (mac.length() > 0 &&
					list.searchAnchorByMacStr(mac.c_str()) != nullptr) {
					ok = storage.mergeAnchorByMac(mac.c_str(), patch);
				} else if (lookupShort != 0) {
					ok = storage.mergeAnchorByShort(lookupShort, patch);
				}
				if (!ok) {
					request->send(500, "application/json",
								  "{\"error\":\"update failed\"}");
					return;
				}
				request->send(200, "application/json", "{\"ok\":true}");
			});

		server.on(
			"/api/sync", HTTP_POST,
			[](AsyncWebServerRequest *request) {
				uint16_t shortAddr = 0;
				if (parseSyncShortParam(request, shortAddr)) {
					if (!handleSyncRequest(request, shortAddr)) {
						return;
					}
					return;
				}
				if (request->contentLength() == 0) {
					handleSyncRequest(request, 0);
				}
			},
			nullptr,
			[](AsyncWebServerRequest *request, uint8_t *data, size_t len,
			   size_t index, size_t total) {
				if (index + len < total) {
					return;
				}
				uint16_t shortAddr = 0;
				DUMPF("Web POST /api/sync (%u bytes)\n", (unsigned)total);
				if (total > 0) {
					JSONVar body = JSON.parse(
						String((const char *)data, (unsigned)total));
					if (JSON.typeof(body) != "undefined" &&
						body.hasOwnProperty("shortAddress")) {
						shortAddr = (uint16_t)(int)body["shortAddress"];
					}
				}
				handleSyncRequest(request, shortAddr);
			});
	}

	static bool handleSyncRequest(AsyncWebServerRequest *request,
								  uint16_t shortAddr) {
		if (WiFi.status() != WL_CONNECTED) {
			JSONVar err;
			err["ok"] = false;
			err["error"] = "wifi_disconnected";
			err["hint"] =
				"Tag sin WiFi. ESP-NOW requiere STA conectado al AP.";
			AsyncWebServerResponse *response = request->beginResponse(
				503, "application/json", JSON.stringify(err));
			addCors(response);
			request->send(response);
			return false;
		}
		if (!IndoorUWB_ESPNow::requestAnchorSync(shortAddr)) {
			JSONVar err;
			err["ok"] = false;
			err["error"] = "espnow_failed";
			AsyncWebServerResponse *response = request->beginResponse(
				503, "application/json", JSON.stringify(err));
			addCors(response);
			request->send(response);
			return false;
		}
		sendSyncResponse(request, shortAddr);
		return true;
	}

	static bool parseSyncShortParam(AsyncWebServerRequest *request,
									uint16_t &shortAddr) {
		if (!request->hasParam("short", true)) {
			return false;
		}
		String s = request->getParam("short", true)->value();
		s.trim();
		if (s.length() == 0) {
			return false;
		}
		if (s.startsWith("0x") || s.startsWith("0X")) {
			shortAddr = (uint16_t)strtoul(s.c_str() + 2, nullptr, 16);
		} else {
			shortAddr = (uint16_t)s.toInt();
		}
		return true;
	}

	static void sendSyncResponse(AsyncWebServerRequest *request,
								 uint16_t shortAddr) {
		JSONVar resp;
		resp["ok"] = true;
		resp["shortAddress"] = shortAddr;
		AsyncWebServerResponse *response = request->beginResponse(
			200, "application/json", JSON.stringify(resp));
		addCors(response);
		request->send(response);
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	String positionToJson() {
		const AnchorPosition &p = IndoorUWB_Storage::getInstance().position;
		IndoorUWB_Storage &storage = IndoorUWB_Storage::getInstance();
		JSONVar info;
		info["role"] = "anchor";
		info["name"] = DEVICE_NAME;
		info["mac"] = WiFi.macAddress();
		info["ip"] = WiFi.localIP().toString();
		info["uwb_address"] = storage.getUwbAddress();
		info["x"] = p.x;
		info["y"] = p.y;
		info["z"] = p.z;
		info["offset"] = p.offset;
		uint8_t bytes[8];
		JSONVar byteArr;
		if (storage.parseUwbAddressToBytes(bytes)) {
			for (int i = 0; i < 8; i++) {
				char hex[3];
				snprintf(hex, sizeof(hex), "%02X", bytes[i]);
				byteArr[i] = hex;
			}
		}
		info["bytes"] = byteArr;
		return JSON.stringify(info);
	}

	String uwbAddressToJson() {
		IndoorUWB_Storage &storage = IndoorUWB_Storage::getInstance();
		JSONVar info;
		info["uwb_address"] = storage.getUwbAddress();
		uint8_t bytes[8];
		JSONVar byteArr;
		if (storage.parseUwbAddressToBytes(bytes)) {
			for (int i = 0; i < 8; i++) {
				char hex[3];
				snprintf(hex, sizeof(hex), "%02X", bytes[i]);
				byteArr[i] = hex;
			}
		}
		info["bytes"] = byteArr;
		return JSON.stringify(info);
	}

	void registerAnchorRoutes() {
		server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse(
				200, "application/json", positionToJson());
			addCors(response);
			request->send(response);
		});

		server.on("/api/position", HTTP_GET, [this](AsyncWebServerRequest *request) {
			AsyncWebServerResponse *response = request->beginResponse(
				200, "application/json", positionToJson());
			addCors(response);
			request->send(response);
		});

		server.on(
			"/api/position", HTTP_POST,
			[](AsyncWebServerRequest *request) {
				if (request->contentLength() == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
				}
			},
			nullptr,
			[](AsyncWebServerRequest *request, uint8_t *data, size_t len,
			   size_t index, size_t total) {
				if (index + len < total) {
					return;
				}
				DUMPF("Web POST /api/position (%u bytes)\n", (unsigned)total);
				if (total == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
					return;
				}
				JSONVar body =
					JSON.parse(String((const char *)data, (unsigned)total));
				if (JSON.typeof(body) == "undefined") {
					request->send(400, "application/json",
								  "{\"error\":\"invalid json\"}");
					return;
				}

				AnchorPosition &pos =
					IndoorUWB_Storage::getInstance().position;
				if (body.hasOwnProperty("x")) {
					pos.x = (double)body["x"];
				}
				if (body.hasOwnProperty("y")) {
					pos.y = (double)body["y"];
				}
				if (body.hasOwnProperty("z")) {
					pos.z = (double)body["z"];
				}
				if (body.hasOwnProperty("offset")) {
					pos.offset = (double)body["offset"];
				}
				IndoorUWB_Storage::getInstance().savePosition();
				request->send(200, "application/json", "{\"ok\":true}");
			});

		server.on("/api/sync", HTTP_POST, [](AsyncWebServerRequest *request) {
			IndoorUWB_ESPNow::sendPositionSync();
			request->send(200, "application/json", "{\"ok\":true}");
		});

		server.on("/api/uwb-address", HTTP_GET,
				  [this](AsyncWebServerRequest *request) {
					  AsyncWebServerResponse *response = request->beginResponse(
						  200, "application/json", uwbAddressToJson());
					  addCors(response);
					  request->send(response);
				  });

		server.on(
			"/api/uwb-address", HTTP_POST,
			[](AsyncWebServerRequest *request) {
				if (request->contentLength() == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
				}
			},
			nullptr,
			[](AsyncWebServerRequest *request, uint8_t *data, size_t len,
			   size_t index, size_t total) {
				if (index + len < total) {
					return;
				}
				DUMPF("Web POST /api/uwb-address (%u bytes)\n", (unsigned)total);
				if (total == 0) {
					request->send(400, "application/json",
								  "{\"error\":\"missing body\"}");
					return;
				}
				JSONVar body =
					JSON.parse(String((const char *)data, (unsigned)total));
				if (JSON.typeof(body) == "undefined") {
					request->send(400, "application/json",
								  "{\"error\":\"invalid json\"}");
					return;
				}
				if (JSON.typeof(body["bytes"]) == "undefined") {
					request->send(400, "application/json",
								  "{\"error\":\"bytes required\"}");
					return;
				}
				uint8_t uwbBytes[8];
				for (int i = 0; i < 8; i++) {
					if (JSON.typeof(body["bytes"][i]) == "undefined") {
						request->send(400, "application/json",
									  "{\"error\":\"8 hex bytes required\"}");
						return;
					}
					String hex = (const char *)body["bytes"][i];
					hex.trim();
					hex.toUpperCase();
					if (!AnchorList::parseUwbByteHex(hex.c_str(), &uwbBytes[i])) {
						request->send(400, "application/json",
									  "{\"error\":\"invalid hex byte\"}");
						return;
					}
				}
				IndoorUWB_Storage::getInstance().saveUwbAddressFromBytes(
					uwbBytes);
				request->send(200, "application/json",
							  "{\"ok\":true,\"reboot\":true}");
				delay(400);
				ESP.restart();
			});
	}
#endif
};

#endif
