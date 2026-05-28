#ifndef _INDOOR_UWB_ANCHOR_TEST_UTILS_H
#define _INDOOR_UWB_ANCHOR_TEST_UTILS_H

#include "../../src/models/AnchorModel.h"

inline Anchor makeTestAnchor(const char *name, uint16_t shortAddr, float x,
							 float y, float z) {
	Anchor a{};
	strncpy(a.name, name, sizeof(a.name) - 1);
	a.shortAddress = shortAddr;
	a.x = x;
	a.y = y;
	a.z = z;
	a.o = 0.f;
	strncpy(a.DW1000_Address, "AA:BB:CC:DD:EE:FF:00:11",
			sizeof(a.DW1000_Address) - 1);
	strncpy(a.MAC_Address, "40:91:51:AE:95:50", sizeof(a.MAC_Address) - 1);
	return a;
}

#endif
