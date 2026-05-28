#ifndef _INDOOR_UWB_DW1000_H
#define _INDOOR_UWB_DW1000_H

#include "../config.h"
#include "../models/AnchorModel.h"
#include "../models/Trilateration.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_Eeprom.h"
#include "IndoorUWB_EspNow.h"
#include <DW1000Ranging.h>
#include <SPI.h>

#if defined(INDOOR_UWB_ROLE_TAG)
extern Vector3D gPosition;
#endif

class IndoorUwb_Dw1000 : public IndoorUwb_Controller {
  public:
	static IndoorUwb_Dw1000 &getInstance() {
		static IndoorUwb_Dw1000 instance;
		return instance;
	}

	ControllerType getControllerType() override {
		return ControllerType::CONTROLLER_DW1000;
	}

	void begin() override {
		SPI.begin(UWB_SPI_SCK, UWB_SPI_MISO, UWB_SPI_MOSI);
		DW1000Ranging.initCommunication(UWB_PIN_RST, UWB_PIN_SS, UWB_PIN_IRQ);
		DW1000Ranging.attachNewRange(onNewRange);
		DW1000Ranging.attachNewDevice(onNewDevice);
		DW1000Ranging.attachInactiveDevice(onInactiveDevice);
#if UWB_RANGE_FILTER
		DW1000Ranging.useRangeFilter(true);
#endif
#if defined(INDOOR_UWB_ROLE_TAG)
		DW1000Ranging.startAsTag(DW1000_ADDRESS,
								 DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		DW1000Ranging.startAsAnchor(ANCHOR_ADDRESS,
									DW1000.MODE_LONGDATA_RANGE_LOWPOWER, false);
#endif
		DUMPSLN("DW1000 ranging started");
	}

	void loop() { DW1000Ranging.loop(); }

  private:
	static void onNewRange() {
#if defined(INDOOR_UWB_ROLE_TAG)
		onNewRangeTag();
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		onNewRangeAnchor();
#endif
	}

	static void onNewDevice(DW1000Device *device) {
#if PRINTDEBUG
		DUMPLN("NEW UWB device short:", device->getShortAddress());
#endif
#if defined(INDOOR_UWB_ROLE_TAG)
		EspNowPosition pkt{};
		IndoorUwb_EspNow::sendPosition(&pkt);
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		EspNowPosition pkt = IndoorUwb_Eeprom::getInstance().position;
		IndoorUwb_EspNow::sendPosition(&pkt);
#endif
		(void)device;
	}

	static void onInactiveDevice(DW1000Device *device) {
		DUMPHEX("UWB inactive: ", device->getShortAddress());
		DUMPPRINTLN();
	}

#if defined(INDOOR_UWB_ROLE_TAG)
	static void onNewRangeTag() {
#if PRINTDEBUG && UWB_DEBUG
		DW1000Device *dev = DW1000Ranging.getDistantDevice();
		if (!dev) {
			return;
		}
		DUMP("Range from ", dev->getShortAddress());
		DUMPLN(" m: ", dev->getRange());
#endif
		if (DW1000Ranging.getNetworkDevicesNumber() < TRILATERATION_NODES) {
			return;
		}
#ifdef TRILATERATION3D
		// Trilateración completa requiere mapear anchors de EEPROM +
		// distancias; se deja hook para extensión (como IndoorUWB original).
#if TRILATERATION_DEBUG
		DUMP("TRILATERATION pos ", gPosition(0));
		DUMP(" ", gPosition(1));
		DUMPLN(" ", gPosition(2));
#endif
#endif
	}
#endif

#if defined(INDOOR_UWB_ROLE_ANCHOR)
	static void onNewRangeAnchor() {
#if PRINTDEBUG && UWB_DEBUG
		DW1000Device *dev = DW1000Ranging.getDistantDevice();
		if (!dev) {
			return;
		}
		DUMP("Range tag ", dev->getShortAddress());
		DUMPLN(" m: ", dev->getRange());
#endif
	}
#endif
};

#if defined(INDOOR_UWB_ROLE_TAG)
Vector3D gPosition;
#endif

#endif
