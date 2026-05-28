#include "wifi_test_helper.h"

#include <string.h>

extern "C" {
#include "esp_wifi.h"
}

bool wifiTestConnected = false;

#if WIFI_TEST_CUSTOM_MAC
void applyTestMacAddress() {
	esp_wifi_set_mac(WIFI_IF_STA, test_mac_address);
}
#endif

bool connectTestWifi() {
	WiFi.persistent(false);
	WiFi.mode(WIFI_STA);
	WiFi.disconnect(true);

	if (!WiFi.config(TEST_WIFI_LOCAL_IP, GATEWAY, SUBNET, PRIMARYDNS,
				   SECONDARYDNS)) {
		DUMPSLN("STA Failed to configure");
	}

#if WIFI_TEST_CUSTOM_MAC
	applyTestMacAddress();
#endif

	WiFi.begin(WIFI_SSID, WIFI_PASS);

	const uint8_t maxRetries = 15;
	const uint32_t pollMs = 1000;
	uint8_t attempt = 0;
	uint32_t lastPollMs = millis();

	while (attempt < maxRetries && WiFi.status() != WL_CONNECTED) {
		if (millis() - lastPollMs >= pollMs) {
			++attempt;
			lastPollMs = millis();
		}
		delay(50);
	}

	wifiTestConnected = (WiFi.status() == WL_CONNECTED);
	return wifiTestConnected;
}
