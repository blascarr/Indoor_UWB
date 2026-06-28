#ifndef _INDOOR_UWB_CONFIG_NETWORK_H
#define _INDOOR_UWB_CONFIG_NETWORK_H

/** Red estática: octetos configurables vía build_flags (-D INDOOR_UWB_*_O1..O4).
 *  Por defecto 192.168.1.0/24, gateway .1. Solo hace falta -D INDOOR_UWB_IP_O4=N
 *  por dispositivo; el resto se hereda del entorno o de estos defaults. */

#ifndef INDOOR_UWB_IP_O1
#define INDOOR_UWB_IP_O1 192
#endif
#ifndef INDOOR_UWB_IP_O2
#define INDOOR_UWB_IP_O2 168
#endif
#ifndef INDOOR_UWB_IP_O3
#define INDOOR_UWB_IP_O3 40
#endif

#ifndef INDOOR_UWB_GW_O1
#define INDOOR_UWB_GW_O1 INDOOR_UWB_IP_O1
#endif
#ifndef INDOOR_UWB_GW_O2
#define INDOOR_UWB_GW_O2 INDOOR_UWB_IP_O2
#endif
#ifndef INDOOR_UWB_GW_O3
#define INDOOR_UWB_GW_O3 INDOOR_UWB_IP_O3
#endif
#ifndef INDOOR_UWB_GW_O4
#define INDOOR_UWB_GW_O4 1
#endif

#ifndef INDOOR_UWB_SUBNET_O1
#define INDOOR_UWB_SUBNET_O1 255
#endif
#ifndef INDOOR_UWB_SUBNET_O2
#define INDOOR_UWB_SUBNET_O2 255
#endif
#ifndef INDOOR_UWB_SUBNET_O3
#define INDOOR_UWB_SUBNET_O3 255
#endif
#ifndef INDOOR_UWB_SUBNET_O4
#define INDOOR_UWB_SUBNET_O4 0
#endif

#define INDOOR_UWB_GATEWAY                                                       \
	IPAddress(INDOOR_UWB_GW_O1, INDOOR_UWB_GW_O2, INDOOR_UWB_GW_O3,            \
			  INDOOR_UWB_GW_O4)
#define INDOOR_UWB_SUBNET                                                        \
	IPAddress(INDOOR_UWB_SUBNET_O1, INDOOR_UWB_SUBNET_O2, INDOOR_UWB_SUBNET_O3, \
			  INDOOR_UWB_SUBNET_O4)

#define INDOOR_UWB_MAKE_IP(o4)                                                 \
	IPAddress(INDOOR_UWB_IP_O1, INDOOR_UWB_IP_O2, INDOOR_UWB_IP_O3, (o4))

#endif
