#ifndef _INDOOR_UWB_TAG_POSITION_H
#define _INDOOR_UWB_TAG_POSITION_H

#include "Trilateration.h"

struct TagPositionState {
	Vector3D pos;
	bool valid = false;
	uint8_t anchorsUsed = 0;
	float residual = -1.f;
	unsigned long updatedMs = 0;
};

#if defined(INDOOR_UWB_ROLE_TAG)
extern TagPositionState gTagPosition;
#endif

inline const char *trilaterationModeStr() {
#if defined(TRILATERATION3D)
	return "3d";
#elif defined(TRILATERATION2D)
	return "2d";
#else
	return "none";
#endif
}

#endif
