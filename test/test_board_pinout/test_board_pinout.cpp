#include <Arduino.h>
#include <unity.h>

#include "../src/test_config.h"

static void test_spi_pins_common() {
	TEST_ASSERT_EQUAL(18, UWB_SPI_SCK);
	TEST_ASSERT_EQUAL(19, UWB_SPI_MISO);
	TEST_ASSERT_EQUAL(23, UWB_SPI_MOSI);
	TEST_ASSERT_EQUAL(27, UWB_PIN_RST);
	TEST_ASSERT_EQUAL(34, UWB_PIN_IRQ);
}

#if defined(INDOOR_UWB_BOARD_MAKERFABS_PRO_DISPLAY)
static void test_chip_select_pro_display() { TEST_ASSERT_EQUAL(21, UWB_PIN_SS); }
#elif defined(INDOOR_UWB_BOARD_MAKERFABS_V1) || defined(INDOOR_UWB_BOARD_ESP32DEV)
static void test_chip_select_makerfabs_v1() { TEST_ASSERT_EQUAL(4, UWB_PIN_SS); }
#endif

static void test_trilateration_node_count() {
	TEST_ASSERT_EQUAL(3, TRILATERATION_NODES);
}

static void test_eeprom_anchor_slots() {
	TEST_ASSERT_EQUAL(4, EEPROM_ANCHORLIST_SIZE);
}

void setup() {
	delay(200);
	UNITY_BEGIN();
	RUN_TEST(test_spi_pins_common);
#if defined(INDOOR_UWB_BOARD_MAKERFABS_PRO_DISPLAY)
	RUN_TEST(test_chip_select_pro_display);
#else
	RUN_TEST(test_chip_select_makerfabs_v1);
#endif
	RUN_TEST(test_trilateration_node_count);
	RUN_TEST(test_eeprom_anchor_slots);
	UNITY_END();
}

void loop() {}
