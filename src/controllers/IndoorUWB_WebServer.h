#ifndef _INDOOR_UWB_WEBSERVER_H
#define _INDOOR_UWB_WEBSERVER_H

#include <Arduino_JSON.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <DW1000Ranging.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

#include "../assets/webpage.h"
#include "../config.h"
#include "../models/AnchorModel.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_Dw1000.h"
#include "IndoorUWB_Eeprom.h"

class IndoorUwb_WebServer : public IndoorUwb_Controller {
  public:
	AsyncWebServer server{SERVER_PORT};
	AsyncEventSource events{"/anchor_info"};
	DNSServer dns;

	static IndoorUwb_WebServer &getInstance() {
		static IndoorUwb_WebServer instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_WEB;
	}

	void begin() override {
#if defined(INDOOR_UWB_ROLE_TAG)
		registerTagRoutes();
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		registerAnchorRoutes();
#endif
		server.addHandler(&events);
		server.begin();
		DUMPSLN("Async web server started");
	}

  private:
#if defined(INDOOR_UWB_ROLE_TAG)
	String nodeInfoJson(AnchorList &list) {
		JSONVar anchorInfo;
		for (int i = 0; i < list.devices; i++) {
			const Anchor &a = list.list[i];
			anchorInfo[i][ANCHOR_NAME_INPUT] = String(a.name);
			anchorInfo[i][MAC_ADDRESS_INPUT] = String(a.MAC_Address);
			anchorInfo[i][DW1000_ADDRESS_INPUT] = String(a.DW1000_Address);
			anchorInfo[i][DW1000_NUM_INPUT] = a.shortAddress;
			anchorInfo[i][X_INPUT] = a.x;
			anchorInfo[i][Y_INPUT] = a.y;
			anchorInfo[i][Z_INPUT] = a.z;
			anchorInfo[i][OFFSET_INPUT] = a.o;
		}
		return JSON.stringify(anchorInfo);
	}

	void registerTagRoutes() {
		AnchorList &anchorList = IndoorUwb_Eeprom::getInstance().anchorList;

		server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->send_P(200, "text/html", indoor_html);
		});
		server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->send_P(200, "text/html", node_html);
		});

		events.onConnect([&anchorList, this](AsyncEventSourceClient *client) {
			(void)client;
			events.send(nodeInfoJson(anchorList).c_str(), NODE_INFO_SEND_EVENT,
						millis());
			events.send(WiFi.macAddress().c_str(), MAC_ADDRESS_SEND_EVENT,
						millis());
		});

		server.on(
			ADD_ANCHOR_INPUT, HTTP_GET,
			[&anchorList](AsyncWebServerRequest *request) {
				if (!request->hasParam(ANCHOR_NAME_INPUT) ||
					!request->hasParam(DW1000_ADDRESS_INPUT) ||
					!request->hasParam(MAC_ADDRESS_INPUT) ||
					!request->hasParam(POSITIONX_INPUT)) {
					request->send(400, "text/plain", "missing params");
					return;
				}
				Anchor na{};
				strncpy(
					na.DW1000_Address,
					request->getParam(DW1000_ADDRESS_INPUT)->value().c_str(),
					sizeof(na.DW1000_Address) - 1);
				strncpy(na.MAC_Address,
						request->getParam(MAC_ADDRESS_INPUT)->value().c_str(),
						sizeof(na.MAC_Address) - 1);
				strncpy(na.name,
						request->getParam(ANCHOR_NAME_INPUT)->value().c_str(),
						sizeof(na.name) - 1);
				na.x = request->getParam(POSITIONX_INPUT)->value().toFloat();
				na.y = request->getParam(POSITIONY_INPUT)->value().toFloat();
				na.z = request->getParam(POSITIONZ_INPUT)->value().toFloat();
				na.o = request->getParam(POSITION_OFFSET)->value().toFloat();
				if (request->hasParam(DW1000_NUM_INPUT)) {
					na.shortAddress =
						(uint16_t)request->getParam(DW1000_NUM_INPUT)
							->value()
							.toInt();
				}
				anchorList.addAnchor(na);
				IndoorUwb_Eeprom::getInstance().saveAnchorList(anchorList);
				request->send(200, "text/plain", "OK");
			});

		server.on(REMOVE_ANCHOR_INPUT, HTTP_GET,
				  [&anchorList, this](AsyncWebServerRequest *request) {
					  String name =
						  request->getParam(REMOVE_ANCHOR_NAME_INPUT)->value();
					  if (anchorList.removeAnchor(name)) {
						  IndoorUwb_Eeprom::getInstance().saveAnchorList(
							  anchorList);
						  events.send(nodeInfoJson(anchorList).c_str(),
									  NODE_INFO_SEND_EVENT, millis());
					  }
					  request->send(200, "text/plain", "OK");
				  });

		server.on(CLEAN_MEMORY_INPUT, HTTP_GET,
				  [&anchorList, this](AsyncWebServerRequest *request) {
					  IndoorUwb_Eeprom::getInstance().clearAnchorList(
						  anchorList);
					  events.send(nodeInfoJson(anchorList).c_str(),
								  NODE_INFO_SEND_EVENT, millis());
					  request->send(200, "text/plain", "OK");
				  });
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	String nodeInfoJson() {
		const EspNowPosition &p = IndoorUwb_Eeprom::getInstance().position;
		JSONVar info;
		info[MAC_ADDRESS_INPUT] = WiFi.macAddress();
		info[X_INPUT] = p.x;
		info[Y_INPUT] = p.y;
		info[Z_INPUT] = p.z;
		info[OFFSET_INPUT] = p.offset;
		return JSON.stringify(info);
	}

	void registerAnchorRoutes() {
		server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
			request->send_P(200, "text/html", anchor_html);
		});

		events.onConnect([this](AsyncEventSourceClient *client) {
			(void)client;
			events.send(nodeInfoJson().c_str(), NODE_INFO_SEND_EVENT, millis());
		});

		server.on(
			POSITION_SERVER_INPUT, HTTP_GET,
			[](AsyncWebServerRequest *request) {
				if (!request->hasParam(POSITIONX_INPUT)) {
					request->send(400, "text/plain", "missing params");
					return;
				}
				EspNowPosition p{};
				p.x = request->getParam(POSITIONX_INPUT)->value().toFloat();
				p.y = request->getParam(POSITIONY_INPUT)->value().toFloat();
				p.z = request->getParam(POSITIONZ_INPUT)->value().toFloat();
				p.offset =
					request->getParam(POSITION_OFFSET)->value().toFloat();
				p.address = (uint16_t)p.x;
				IndoorUwb_Eeprom::getInstance().position = p;
				IndoorUwb_Eeprom::getInstance().savePosition(p);
				request->send(200, "text/plain", "OK");
			});
	}
#endif
};

#endif
