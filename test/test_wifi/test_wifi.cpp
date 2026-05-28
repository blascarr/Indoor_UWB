#include <Arduino.h>
#include <unity.h>

#include "../src/test_config.h"
#include "../utils/wifi_test_helper.h"

static void assertIpEqual(const IPAddress &expected, const IPAddress &actual,
						  const char *label) {
	TEST_ASSERT_TRUE_MESSAGE(expected == actual, label);
}

static void test_wifi_connection() {
	TEST_ASSERT_TRUE_MESSAGE(connectTestWifi(),
							"No se pudo conectar al AP; revisa WIFI_SSID en "
							"config.h");
}

static void test_wifi_local_ip() {
	assertIpEqual(TEST_WIFI_LOCAL_IP, WiFi.localIP(), "localIP mismatch");
}

static void test_wifi_gateway() {
	assertIpEqual(GATEWAY, WiFi.gatewayIP(), "gateway mismatch");
}

static void test_wifi_subnet() {
	assertIpEqual(SUBNET, WiFi.subnetMask(), "subnet mismatch");
}

#if WIFI_TEST_CUSTOM_MAC
static void test_wifi_custom_mac() {
	uint8_t mac[6];
	WiFi.macAddress(mac);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(test_mac_address, mac, 6);
}
#endif

void setup() {
	if (PRINTDEBUG) {
		Serial.begin(115200);
		delay(500);
	}

	UNITY_BEGIN();
	RUN_TEST(test_wifi_connection);
	RUN_TEST(test_wifi_local_ip);
	RUN_TEST(test_wifi_gateway);
	RUN_TEST(test_wifi_subnet);
#if WIFI_TEST_CUSTOM_MAC
	RUN_TEST(test_wifi_custom_mac);
#endif
	UNITY_END();
}

void loop() {}
