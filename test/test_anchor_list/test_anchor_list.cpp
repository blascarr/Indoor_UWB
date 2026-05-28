#include <Arduino.h>
#include <unity.h>

#include "../helpers/anchor_test_utils.h"
#include "../../src/models/AnchorModel.h"

static void test_anchor_list_starts_empty() {
	AnchorList list;
	TEST_ASSERT_EQUAL(0, list.devices);
	TEST_ASSERT_EQUAL(ANCHORLIST_FINGERPRINT, list.fingerprint);
}

static void test_add_anchor_increments_devices() {
	AnchorList list;
	list.addAnchor(makeTestAnchor("A1", 1, 0.f, 0.f, 0.f));
	TEST_ASSERT_EQUAL(1, list.devices);
	TEST_ASSERT_EQUAL_STRING("A1", list.list[0].name);
	TEST_ASSERT_EQUAL_UINT16(1, list.list[0].shortAddress);
}

static void test_search_by_short_address() {
	AnchorList list;
	list.addAnchor(makeTestAnchor("A1", 10, 1.f, 2.f, 3.f));
	list.addAnchor(makeTestAnchor("A2", 20, 4.f, 5.f, 6.f));

	Anchor *found = list.searchAnchorByShortAddress(20);
	TEST_ASSERT_NOT_NULL(found);
	TEST_ASSERT_EQUAL_STRING("A2", found->name);

	TEST_ASSERT_NULL(list.searchAnchorByShortAddress(99));
}

static void test_remove_anchor_by_name() {
	AnchorList list;
	list.addAnchor(makeTestAnchor("North", 1, 0.f, 5.f, 0.f));
	list.addAnchor(makeTestAnchor("South", 2, 0.f, 0.f, 0.f));

	TEST_ASSERT_TRUE(list.removeAnchor(String("North")));
	TEST_ASSERT_EQUAL(1, list.devices);
	TEST_ASSERT_EQUAL_STRING("South", list.list[0].name);

	TEST_ASSERT_FALSE(list.removeAnchor(String("East")));
}

static void test_anchor_list_capacity_limit() {
	AnchorList list;
	for (uint8_t i = 0; i < EEPROM_ANCHORLIST_SIZE + 2; i++) {
		char name[6];
		snprintf(name, sizeof(name), "A%u", i);
		list.addAnchor(makeTestAnchor(name, i, (float)i, 0.f, 0.f));
	}
	TEST_ASSERT_EQUAL(EEPROM_ANCHORLIST_SIZE, list.devices);
}

static void test_clean_list() {
	AnchorList list;
	list.addAnchor(makeTestAnchor("X", 1, 0.f, 0.f, 0.f));
	list.cleanList();
	TEST_ASSERT_EQUAL(0, list.devices);
}

static void test_espnow_position_is_pod() {
	/** uint16_t + 4×float; alineación ESP32 suele dar 20 bytes (sin String). */
	TEST_ASSERT_TRUE(sizeof(EspNowPosition) >= 18);
	TEST_ASSERT_TRUE(sizeof(EspNowPosition) <= 24);
}

void setup() {
	delay(200);
	UNITY_BEGIN();
	RUN_TEST(test_anchor_list_starts_empty);
	RUN_TEST(test_add_anchor_increments_devices);
	RUN_TEST(test_search_by_short_address);
	RUN_TEST(test_remove_anchor_by_name);
	RUN_TEST(test_anchor_list_capacity_limit);
	RUN_TEST(test_clean_list);
	RUN_TEST(test_espnow_position_is_pod);
	UNITY_END();
}

void loop() {}
