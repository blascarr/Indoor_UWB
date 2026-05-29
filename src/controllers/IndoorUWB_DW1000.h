#ifndef _INDOOR_UWB_DW1000_H
#define _INDOOR_UWB_DW1000_H

#include "../config.h"
#include "../models/AnchorModel.h"
#include "../models/Trilateration.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_ESPNow.h"
#include "IndoorUWB_Storage.h"
#include <DW1000Ranging.h>
#include <SPI.h>

#if defined(INDOOR_UWB_ROLE_TAG)
extern Vector3D gPosition;
#endif

class IndoorUWB_DW1000 : public IndoorUWB_Controller {
  public:
	static IndoorUWB_DW1000 &getInstance() {
		static IndoorUWB_DW1000 instance;
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
		DUMPF("UWB: nuevo dispositivo, short=0x%04X\n",
			  device->getShortAddress());
#if defined(INDOOR_UWB_ROLE_TAG)
		IndoorUWB_ESPNow::requestAnchorSync(device->getShortAddress());
#endif
	}

	static void onInactiveDevice(DW1000Device *device) {
		DUMPF("UWB: dispositivo inactivo, short=0x%04X\n",
			  device->getShortAddress());
	}

#if defined(INDOOR_UWB_ROLE_TAG)
	static void onNewRangeTag() {
		DW1000Device *dev = DW1000Ranging.getDistantDevice();
		if (dev) {
			DUMPF("UWB: rango anchor 0x%04X = %.2f m\n", dev->getShortAddress(),
				  dev->getRange());
		}
		if (DW1000Ranging.getNetworkDevicesNumber() < TRILATERATION_NODES) {
			return;
		}
#ifdef TRILATERATION3D
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
		DW1000Device *dev = DW1000Ranging.getDistantDevice();
		if (dev) {
			DUMPF("UWB: rango con tag 0x%04X = %.2f m\n", dev->getShortAddress(),
				  dev->getRange());
		}
	}

#endif
};

#if defined(INDOOR_UWB_ROLE_TAG)
Vector3D gPosition;
#endif

#endif
