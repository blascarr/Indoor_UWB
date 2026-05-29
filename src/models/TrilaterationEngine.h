#ifndef _INDOOR_UWB_TRILATERATION_ENGINE_H
#define _INDOOR_UWB_TRILATERATION_ENGINE_H

#include "../config.h"
#include "../controllers/IndoorUWB_Storage.h"
#include "AnchorModel.h"
#include "TagPosition.h"
#include "Trilateration.h"
#include <DW1000Ranging.h>

struct TrilaterationSample {
	Vector3D P[4];
	float d[4];
	uint8_t count = 0;
};

inline bool collectTrilaterationSample(TrilaterationSample &sample) {
	AnchorList &list = IndoorUWB_Storage::getInstance().anchorList;
	sample.count = 0;
	const uint8_t n = DW1000Ranging.getNetworkDevicesNumber();
	for (uint8_t i = 0; i < n && sample.count < 4; i++) {
		DW1000Device *dev = DW1000Ranging.getNetworkDeviceAt(i);
		if (!dev || dev->isInactive()) {
			continue;
		}
		Anchor *anchor =
			list.searchAnchorByShortAddress(dev->getShortAddress());
		if (anchor == nullptr || anchor->shortAddress == 0) {
			continue;
		}
		const float raw = dev->getRange();
		if (raw < UWB_RANGE_MIN_M) {
			continue;
		}
		const float corrected =
			IndoorUWB_Storage::getInstance().correctedRange(
				dev->getShortAddress(), raw);
		if (corrected < 0.f) {
			continue;
		}
		sample.P[sample.count](0, 0) = anchor->x;
		sample.P[sample.count](1, 0) = anchor->y;
		sample.P[sample.count](2, 0) = anchor->z;
		sample.d[sample.count] = corrected;
		sample.count++;
	}
	return sample.count >= TRILATERATION_NODES;
}

inline void runTrilateration(TagPositionState &state) {
	TrilaterationSample sample{};
	if (!collectTrilaterationSample(sample)) {
		state.valid = false;
		return;
	}

	Vector3D result;
	bool ok = false;

#if defined(TRILATERATION3D)
	if (sample.count >= 4) {
		result = trilateration3D(sample.P, sample.d);
		ok = true;
	} else if (sample.count >= 3) {
		ok = trilateration2D(sample.P, sample.d, result);
	}
#elif defined(TRILATERATION2D)
	if (sample.count >= 3) {
		ok = trilateration2D(sample.P, sample.d, result);
	}
#endif

	if (!ok) {
		state.valid = false;
		state.anchorsUsed = sample.count;
		return;
	}

	state.pos = result;
	state.valid = true;
	state.anchorsUsed = sample.count;
	state.residual =
		trilaterationResidual(result, sample.P, sample.d, sample.count);
	state.updatedMs = millis();
}

#endif
