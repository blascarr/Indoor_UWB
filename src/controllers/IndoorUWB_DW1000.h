#ifndef _INDOOR_UWB_DW1000_H
#define _INDOOR_UWB_DW1000_H

#include "../config.h"
#include "../models/AnchorModel.h"
#include "../models/TagPosition.h"
#include "../models/Trilateration.h"
#include "../models/TrilaterationEngine.h"
#include "IndoorUWB_Controller.h"
#include "IndoorUWB_ESPNow.h"
#include "IndoorUWB_Storage.h"
#include <DW1000Ranging.h>
#include <SPI.h>

#if defined(INDOOR_UWB_ROLE_TAG)
extern Vector3D gPosition;
extern TagPositionState gTagPosition;
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
		applyRangingConfig();
		DW1000Ranging.attachNewRange(onNewRange);
		DW1000Ranging.attachNewDevice(onNewDevice);
		DW1000Ranging.attachInactiveDevice(onInactiveDevice);
#if defined(INDOOR_UWB_ROLE_TAG)
		DW1000Ranging.startAsTag(DW1000_ADDRESS, uwbRfMode());
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		DW1000Ranging.startAsAnchor(
			IndoorUWB_Storage::getInstance().getUwbAddress(), uwbRfMode(),
			false);
#endif
		DUMPSLN("DW1000 ranging started");
		setCallback([this]() { loop(); }, UWB_LOOP_INTERVAL_MS, 0, MILLIS);
		start();
	}

	void loop() { DW1000Ranging.loop(); }

	void update() { pollTicker(); }

  private:
	static const byte *uwbRfMode() {
#if UWB_RF_MODE_ACCURACY
		return DW1000.MODE_LONGDATA_RANGE_ACCURACY;
#else
		return DW1000.MODE_LONGDATA_RANGE_LOWPOWER;
#endif
	}

	static void applyRangingConfig() {
		DW1000Ranging.setBaseReplyDelayTime(UWB_REPLY_DELAY_US);
		DW1000Ranging.setBaseTimerDelayMs(UWB_TIMER_DELAY_MS);
		DW1000Ranging.setResetPeriod(UWB_RESET_PERIOD_MS);
		DW1000Ranging.configureRangeValidation(UWB_RANGE_MIN_M, UWB_RANGE_MAX_M,
											  UWB_RANGE_WARMUP_SAMPLES);
#if UWB_RANGE_FILTER
		DW1000Ranging.useRangeFilter(true);
		DW1000Ranging.setRangeFilterValue(UWB_RANGE_FILTER_SIZE);
#else
		DW1000Ranging.useRangeFilter(false);
#endif
	}

	static void onNewRange() {
#if defined(INDOOR_UWB_ROLE_TAG)
		onNewRangeTag();
#elif defined(INDOOR_UWB_ROLE_ANCHOR)
		onNewRangeAnchor();
#endif
	}

	static void onNewDevice(DW1000Device *device) {
		DW1000Ranging.resetDeviceRangeWarmup(device);
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
			const float raw = dev->getRange();
			const float offset =
				IndoorUWB_Storage::getInstance().lookupOffsetByShort(
					dev->getShortAddress());
			const float corrected = raw + offset;
			DUMPF("UWB: rango anchor 0x%04X = %.2f m (raw %.2f, offset %+.2f)\n",
				  dev->getShortAddress(), corrected, raw, offset);
		}
		if (DW1000Ranging.getNetworkDevicesNumber() < TRILATERATION_NODES) {
			return;
		}
		runTrilateration(gTagPosition);
		gPosition = gTagPosition.pos;
#if TRILATERATION_DEBUG
		if (gTagPosition.valid) {
			DUMPF("TRILATERATION pos %.2f %.2f %.2f (anchors=%u residual=%.3f)\n",
				  gPosition(0, 0), gPosition(1, 0), gPosition(2, 0),
				  gTagPosition.anchorsUsed, gTagPosition.residual);
		}
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
TagPositionState gTagPosition;
#endif

#endif
