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

static void test_search_by_mac() {
	AnchorList list;
	Anchor a = makeTestAnchor("North", 1, 0.f, 0.f, 0.f);
	strncpy(a.MAC_Address, "40:91:51:AE:95:74", sizeof(a.MAC_Address) - 1);
	list.addAnchor(a);

	uint8_t mac[] = {0x40, 0x91, 0x51, 0xAE, 0x95, 0x74};
	TEST_ASSERT_NOT_NULL(list.searchAnchorByMac(mac));
	TEST_ASSERT_NULL(list.searchAnchorByMacStr("00:00:00:00:00:01"));
}

static void test_upsert_overwrites_position() {
	AnchorList list;
	Anchor a = makeTestAnchor("A1", 5, 1.f, 2.f, 3.f);
	strncpy(a.MAC_Address, "40:91:51:AE:95:74", sizeof(a.MAC_Address) - 1);
	list.upsertAnchor(a);

	Anchor updated = a;
	updated.x = 9.f;
	updated.y = 8.f;
	updated.z = 7.f;
	TEST_ASSERT_TRUE(list.upsertAnchor(updated));
	TEST_ASSERT_EQUAL(1, list.devices);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.f, list.list[0].x);
	TEST_ASSERT_EQUAL_UINT16(5, list.list[0].shortAddress);
}

static void test_upsert_by_short_without_mac() {
	AnchorList list;
	Anchor a = makeTestAnchor("B", 0x1550, 10.f, 0.f, 0.f);
	a.MAC_Address[0] = '\0';
	TEST_ASSERT_TRUE(list.upsertAnchorEntry(a));
	TEST_ASSERT_EQUAL(1, list.devices);
	TEST_ASSERT_EQUAL_UINT16(0x1550, list.list[0].shortAddress);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.f, list.list[0].x);

	Anchor updated = a;
	updated.x = 12.f;
	updated.o = 0.2f;
	TEST_ASSERT_TRUE(list.upsertAnchorEntry(updated));
	TEST_ASSERT_EQUAL(1, list.devices);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.f, list.list[0].x);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.2f, list.list[0].o);
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

static void test_remove_anchor_by_mac() {
	AnchorList list;
	Anchor a = makeTestAnchor("North", 1, 0.f, 0.f, 0.f);
	strncpy(a.MAC_Address, "40:91:51:AE:95:74", sizeof(a.MAC_Address) - 1);
	list.addAnchor(a);

	TEST_ASSERT_TRUE(list.removeAnchorByMac("40:91:51:AE:95:74"));
	TEST_ASSERT_EQUAL(0, list.devices);
}

static void test_anchor_list_capacity_limit() {
	AnchorList list;
	for (uint8_t i = 0; i < EEPROM_ANCHORLIST_SIZE + 2; i++) {
		char name[6];
		snprintf(name, sizeof(name), "A%u", i);
		Anchor a = makeTestAnchor(name, i, (float)i, 0.f, 0.f);
		snprintf(a.MAC_Address, sizeof(a.MAC_Address),
				 "40:91:51:AE:95:%02X", i);
		list.upsertAnchor(a);
	}
	TEST_ASSERT_EQUAL(EEPROM_ANCHORLIST_SIZE, list.devices);
}

static void test_clean_list() {
	AnchorList list;
	list.addAnchor(makeTestAnchor("X", 1, 0.f, 0.f, 0.f));
	list.cleanList();
	TEST_ASSERT_EQUAL(0, list.devices);
}

static void test_espnow_packet_is_pod() {
	TEST_ASSERT_EQUAL(36, sizeof(EspNowAnchorPacket));
}

void setup() {
	delay(200);
	UNITY_BEGIN();
	RUN_TEST(test_anchor_list_starts_empty);
	RUN_TEST(test_add_anchor_increments_devices);
	RUN_TEST(test_search_by_short_address);
	RUN_TEST(test_search_by_mac);
	RUN_TEST(test_upsert_overwrites_position);
	RUN_TEST(test_upsert_by_short_without_mac);
	RUN_TEST(test_remove_anchor_by_name);
	RUN_TEST(test_remove_anchor_by_mac);
	RUN_TEST(test_anchor_list_capacity_limit);
	RUN_TEST(test_clean_list);
	RUN_TEST(test_espnow_packet_is_pod);
	UNITY_END();
}

void loop() {}
