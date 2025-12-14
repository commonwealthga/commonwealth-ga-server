#include "src/TcpServer/TcpSession/TcpSession.hpp"

void TcpSession::send_inventory_response(int nPawnId) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_INVENTORY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::PAWN_ID, 998);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
	append(response, 0x01, 0x00);        // count elements

		// append(response, 0x0E, 0x00);  // inner item count
		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1); // 0=ok, 1=edited, 2=deleted
		Write4B(response, GA_T::ITEM_ID, 7032); // 0x2DA
		Write4B(response, GA_T::INVENTORY_ID, 999); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		double ts = 1700000000.0;
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, ts);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 7032); // 0x206

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x02, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 999);
			Write4B(response, GA_T::EFFECT_GROUP_ID, 0x0);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x06, 0x00);        // count elements

			append(response, 0x04, 0x00);  // inner item count

			// Write4B(response, GA_T::CHARACTER_ID, 373);
			Write4B(response, GA_T::CHARACTER_ID, nPawnId);
			Write4B(response, GA_T::INVENTORY_ID, 999);
			Write4B(response, GA_T::PROFILE_ID, 0x0);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 5);

			append(response, 0x04, 0x00);  // inner item count

			// Write4B(response, GA_T::CHARACTER_ID, 373);
			Write4B(response, GA_T::CHARACTER_ID, nPawnId);
			Write4B(response, GA_T::INVENTORY_ID, 999);
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 5);

			append(response, 0x04, 0x00);  // inner item count

			// Write4B(response, GA_T::CHARACTER_ID, 373);
			Write4B(response, GA_T::CHARACTER_ID, nPawnId);
			Write4B(response, GA_T::INVENTORY_ID, 999);
			Write4B(response, GA_T::PROFILE_ID, 0x2);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 5);

			append(response, 0x04, 0x00);  // inner item count

			// Write4B(response, GA_T::CHARACTER_ID, 373);
			Write4B(response, GA_T::CHARACTER_ID, nPawnId);
			Write4B(response, GA_T::INVENTORY_ID, 999);
			Write4B(response, GA_T::PROFILE_ID, 0x3);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 5);

			append(response, 0x04, 0x00);  // inner item count

			// Write4B(response, GA_T::CHARACTER_ID, 373);
			Write4B(response, GA_T::CHARACTER_ID, nPawnId);
			Write4B(response, GA_T::INVENTORY_ID, 999);
			Write4B(response, GA_T::PROFILE_ID, 0x4);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 5);

			append(response, 0x04, 0x00);  // inner item count

			// Write4B(response, GA_T::CHARACTER_ID, 373);
			Write4B(response, GA_T::CHARACTER_ID, nPawnId);
			Write4B(response, GA_T::INVENTORY_ID, 999);
			Write4B(response, GA_T::PROFILE_ID, 0x5);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 5);

	send_response(response);
}

