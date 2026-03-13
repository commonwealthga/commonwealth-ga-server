#include "src/TcpServer/TcpSession/TcpSession.hpp"

void TcpSession::send_get_ticket_info_response() {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GET_TICKET_INFO;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write1B(response, GA_T::TEAM_LEADER_FLAG, 0x1);
	Write4B(response, GA_T::CURRENT_MATCH_QUEUE_ID, 0x0);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);

	append(response, 0x9, 0x00);        // count elements (10 entries)


		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000005);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a1); // Low security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a0);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000404);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000006);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1f7); // Medium security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1f6);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000405);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000007);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1f9); // High security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1f8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000406);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// append(response, 0x1c, 0x00);  // inner item count
		// 	Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
		// 	Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000008);
		// 	Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1fa); // Maximum security
		// 	Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1f9);
		// 	Write4B(response, GA_T::ICON_ID, 0x00000219);
		// 	Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
		// 	Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
		// 	Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
		// 	Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
		// 	Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
		// 	Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
		// 	Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
		// 	Write4B(response, GA_T::TAB, 0x000001bb);
		// 	WriteFloat(response, GA_T::MAP_X, 6.0f);
		// 	Write4B(response, GA_T::MAP_Y, 0x00);
		// 	WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
		// 	Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
		// 	Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
		// 	Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
		// 	WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
		// 	Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
		// 	Write4B(response, GA_T::SORT_ORDER, 0x00000000);
		// 	WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
		// 	Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000407);
		// 	WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
		// 	append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
		// 	append(response, 0x00, 0x00);        // count elements
		// 	Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000001);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a9); // Ultra max security - commonwealth prime
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000005bf);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x0000000a);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a9); // Ultra max security - sonoran desert
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005cb);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000005bf);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x0000000b);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a9); // Ultra max security - mining province
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c6);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000005bf);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fe);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000002);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a200); // Mercenary
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1ff);
			Write4B(response, GA_T::ICON_ID, 0x00000214);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x00000003);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x00000001);
			WriteFloat(response, GA_T::MAP_X, 1.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x0000171a);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x00000000);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000000);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000003);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1fc); // Double Agent
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1fb);
			Write4B(response, GA_T::ICON_ID, 0x000001c9);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x00000004);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x00000004);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000000e8);
			WriteFloat(response, GA_T::MAP_X, 10.0f);
			WriteFloat(response, GA_T::MAP_Y, 0.5f);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000004);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000004ec);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			// Write4B(response, GA_T::REMAINING_SECONDS, 0x00000001);
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);        // type 010C
			append(response, 0x00, 0x00);        // count elements

			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000005ae);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000004);
			Write4B(response, GA_T::NAME_MSG_ID, 0x00008fdc); // Sonoran Raid
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000e7b0);
			Write4B(response, GA_T::ICON_ID, 0x000006b4);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x00000015);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x00000015);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x0000003c);
			Write4B(response, GA_T::TAB, 0x000000e7);
			Write4B(response, GA_T::MAP_X, 0x00);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x00000000);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000004eb);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			// Write4B(response, GA_T::REMAINING_SECONDS, 0x00000001);
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

	send_response(response);

		// WriteNBytes(response,	GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x6E});
  // CMarshal__get_int32_t(param_1,MATCH_QUEUE_ID = 0x0337,(uint *)this);
  // CMarshal__get_int32_t(pvVar3,SYS_SITE_ID = 0x04DA,(uint *)((int)this + 4));
  // CMarshal__get_int(pvVar3,QUEUE_TYPE_VALUE_ID = 0x0408,(int *)((int)this + 0x20));
  // CMarshal__get_int32_t(pvVar3,STATUS_MSG_ID = 0x04BC,(uint *)((int)this + 0x14));
  // CMarshal__get_int32_t(pvVar3,NAME_MSG_ID = 0x0371,(uint *)((int)this + 8));
  // CMarshal__get_int32_t(pvVar3,DESC_MSG_ID = 0x01F9,(uint *)((int)this + 0xc));
  // CMarshal__get_int32_t(pvVar3,ICON_ID = 0x02BF,(uint *)((int)this + 0x18));
  // CMarshal__get_int32_t(pvVar3,PLAYER_COUNT = 0x03C2,(uint *)((int)this + 0x2c));
  // CMarshal__get_int32_t(pvVar3,MAX_PLAYERS_PER_SIDE = 0x0346,(uint *)((int)this + 0x34));
  // CMarshal__get_int32_t(pvVar3,MIN_PLAYERS_PER_TEAM = 0x035C,(uint *)((int)this + 0x3c));
  // CMarshal__get_int32_t(pvVar3,MAX_PLAYERS_PER_TEAM = 0x0347,(uint *)((int)this + 0x38));
  // CMarshal__get_int32_t(pvVar3,INSTANCE_COUNT = 0x02C5,(uint *)((int)this + 0x30));
  // CMarshal__get_int32_t(pvVar3,LEVEL_MIN = 0x0306,(uint *)((int)this + 0x40));
  // CMarshal__get_int32_t(pvVar3,LEVEL_MAX = 0x0305,(uint *)((int)this + 0x44));
  // CMarshal__get_float(pvVar3,MAP_X = 0x0331,(float *)((int)this + 0x54));
  // CMarshal__get_float(pvVar3,MAP_Y = 0x0332,(float *)((int)this + 0x58));
  // CMarshal__get_bool(pvVar3,MAP_ACTIVE_FLAG = 0x031F,(undefined1 *)((int)this + 0x5e));
  // CMarshal__get_int32_t(pvVar3,MAP_ICON_TEXTURE_RES_ID = 0x0323,(uint *)((int)this + 0x1c));
  // CMarshal__get_int32_t(pvVar3,VIDEO_RES_ID = 0x0543,(uint *)((int)this + 0x68));
  // CMarshal__get_int32_t(pvVar3,LOCATION_VALUE_ID = 0x0310,(uint *)((int)this + 0x50));
  // CMarshal__get_bool(pvVar3,LOCKED_FLAG = 0x0312,(undefined1 *)((int)this + 0x5f));
  // CMarshal__get_bool(pvVar3,DOUBLE_AGENT_FLAG = 0x021B,(undefined1 *)((int)this + 0x60));
  // CMarshal__get_int32_t(pvVar3,SORT_ORDER = 0x048E,(uint *)((int)this + 0x48));
  // CMarshal__get_bool(pvVar3,BONUS_QUEUE_FLAG = 0x0081,(undefined1 *)((int)this + 0x61));
  // CMarshal__get_ulonglong(pvVar3,ACCESS_FLAGS = 0x0008,(ulonglong *)((int)this + 0x6c));
  // CMarshal__get_int32_t(pvVar3,DIFFICULTY_VALUE_ID = 0x0210,(uint *)((int)this + 0x4c));
  // CMarshal__get_bool(pvVar3,ACTIVE_FLAG = 0x0016,(undefined1 *)((int)this + 0x5d));
  // CMarshal__get_int32_t(pvVar3,REMAINING_SECONDS = 0x0429,&local_8);

}

void TcpSession::send_character_inventory_response(int nPawnId) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_CHARACTER_INVENTORY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);


	append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
	append(response, 0x01, 0x00);        // count elements

		append(response, 0x02, 0x00);  // inner item count

		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP);
		Write4B(response, GA_T::EFFECT_GROUP_ID, 0x0);

	append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
	append(response, 0x06, 0x00);        // count elements

		append(response, 0x04, 0x00);  // inner item count

		Write4B(response, GA_T::CHARACTER_ID, 0); // 0 = applies to any pawn (non-zero would be filtered out by CGameClient__SendInventory)
		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP);
		Write4B(response, GA_T::PROFILE_ID, 0x0);
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 201); // 0xC9 = jetpack slot value ID → maps to equip point 5

		append(response, 0x04, 0x00);  // inner item count

		Write4B(response, GA_T::CHARACTER_ID, 0);
		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP);
		Write4B(response, GA_T::PROFILE_ID, 0x1);
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 201);

		append(response, 0x04, 0x00);  // inner item count

		Write4B(response, GA_T::CHARACTER_ID, 0);
		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP);
		Write4B(response, GA_T::PROFILE_ID, 0x2);
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 201);

		append(response, 0x04, 0x00);  // inner item count

		Write4B(response, GA_T::CHARACTER_ID, 0);
		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP);
		Write4B(response, GA_T::PROFILE_ID, 0x3);
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 201);

		append(response, 0x04, 0x00);  // inner item count

		Write4B(response, GA_T::CHARACTER_ID, 0);
		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP);
		Write4B(response, GA_T::PROFILE_ID, 0x4);
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 201);

		append(response, 0x04, 0x00);  // inner item count

		Write4B(response, GA_T::CHARACTER_ID, 0);
		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP);
		Write4B(response, GA_T::PROFILE_ID, 0x5);
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 201);

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, 0x01, 0x00);        // count elements

		append(response, 0x9, 0x00);  // inner item count

		Write4B(response, GA_T::ITEM_ID, GA_G::DEVICE_ID_MEDIC_CJP); // 0x2DA
		Write4B(response, GA_T::INVENTORY_ID, GA_G::DEVICE_ID_MEDIC_CJP); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		double ts = 1700000000.0;
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, ts);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5

	send_response(response);
}

void TcpSession::send_inventory_response(int nPawnId) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_INVENTORY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
	append(response, 0x07, 0x00);        // count elements

		// append(response, 0x0E, 0x00);  // inner item count
		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1); // 0=ok, 1=edited, 2=deleted
		Write4B(response, GA_T::ITEM_ID, GA_G::DEVICE_ID_MEDIC_CJP); // 0x2DA <<< seems to represent device id
		Write4B(response, GA_T::INVENTORY_ID, 999); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 999); // 0x206 = m_nDeviceInstanceId (must match Device->r_nDeviceInstanceId on server)

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x02, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 999); // must match nInventoryId
			Write4B(response, GA_T::EFFECT_GROUP_ID, 26173);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x04, 0x00);  // inner item count

			Write4B(response, GA_T::CHARACTER_ID, 0); // 0 = applies to any pawn
			Write4B(response, GA_T::INVENTORY_ID, 999); // must match nInventoryId
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 201); // 0xC9 → equip point 5 (jetpack)


		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1); // 0=ok, 1=edited, 2=deleted
		Write4B(response, GA_T::ITEM_ID, GA_G::DEVICE_ID_AGONIZER); // 0x2DA <<< seems to represent device id
		Write4B(response, GA_T::INVENTORY_ID, 1000); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 1000); // 0x206 = m_nDeviceInstanceId (must match Device->r_nDeviceInstanceId on server)

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x02, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 1000); // must match nInventoryId
			Write4B(response, GA_T::EFFECT_GROUP_ID, 16670);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x04, 0x00);  // inner item count

			Write4B(response, GA_T::CHARACTER_ID, 0); // 0 = applies to any pawn
			Write4B(response, GA_T::INVENTORY_ID, 1000); // must match nInventoryId
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 198); // 198 = ranged weapon slot value ID → equip point 2


		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1); // 0=ok, 1=edited, 2=deleted
		Write4B(response, GA_T::ITEM_ID, 864); // 0x2DA <<< seems to represent device id
		Write4B(response, GA_T::INVENTORY_ID, 998); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 998); // 0x206 = m_nDeviceInstanceId (must match Device->r_nDeviceInstanceId on server)

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x02, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 998); // must match nInventoryId
			Write4B(response, GA_T::EFFECT_GROUP_ID, 0);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x04, 0x00);  // inner item count

			Write4B(response, GA_T::CHARACTER_ID, 0); // 0 = applies to any pawn
			Write4B(response, GA_T::INVENTORY_ID, 998); // must match nInventoryId
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 502); // 0xC9 → equip point 5 (jetpack)



		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1); // 0=ok, 1=edited, 2=deleted
		Write4B(response, GA_T::ITEM_ID, 2531); // 0x2DA <<< seems to represent device id
		Write4B(response, GA_T::INVENTORY_ID, 997); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 997); // 0x206 = m_nDeviceInstanceId (must match Device->r_nDeviceInstanceId on server)

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x02, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 997); // must match nInventoryId
			Write4B(response, GA_T::EFFECT_GROUP_ID, 16653);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x04, 0x00);  // inner item count

			Write4B(response, GA_T::CHARACTER_ID, 0); // 0 = applies to any pawn
			Write4B(response, GA_T::INVENTORY_ID, 997); // must match nInventoryId
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 203); // 0xC9 → equip point 5 (jetpack)



		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1); // 0=ok, 1=edited, 2=deleted
		Write4B(response, GA_T::ITEM_ID, 5800); // 0x2DA <<< seems to represent device id
		Write4B(response, GA_T::INVENTORY_ID, 996); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 996); // 0x206 = m_nDeviceInstanceId (must match Device->r_nDeviceInstanceId on server)

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x02, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 996); // must match nInventoryId
			Write4B(response, GA_T::EFFECT_GROUP_ID, 22334);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x04, 0x00);  // inner item count

			Write4B(response, GA_T::CHARACTER_ID, 0); // 0 = applies to any pawn
			Write4B(response, GA_T::INVENTORY_ID, 996); // must match nInventoryId
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 221); // 0xC9 → equip point 5 (jetpack)


		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1); // 0=ok, 1=edited, 2=deleted
		Write4B(response, GA_T::ITEM_ID, 2906); // 0x2DA <<< seems to represent device id
		Write4B(response, GA_T::INVENTORY_ID, 995); // 0x2CF
		Write4B(response, GA_T::BLUEPRINT_ID, 0); // 0x77
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162); // 0xE7
		Write4B(response, GA_T::DURABILITY, 100); // 0x220
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369); // 0x310
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1); // 0x2C5
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 995); // 0x206 = m_nDeviceInstanceId (must match Device->r_nDeviceInstanceId on server)

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x02, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 995); // must match nInventoryId
			Write4B(response, GA_T::EFFECT_GROUP_ID, 9071);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);        // count elements

			append(response, 0x04, 0x00);  // inner item count

			Write4B(response, GA_T::CHARACTER_ID, 0); // 0 = applies to any pawn
			Write4B(response, GA_T::INVENTORY_ID, 995); // must match nInventoryId
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 200); // → equip point 3 (specialty)


		append(response, 0xE, 0x00);  // inner item count

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, 2773); // heal boost device id
		Write4B(response, GA_T::INVENTORY_ID, 994); // must match nInventoryId
		Write4B(response, GA_T::BLUEPRINT_ID, 0);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162);
		Write4B(response, GA_T::DURABILITY, 100);
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, 994); // must match Device->r_nDeviceInstanceId on server

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);

			append(response, 0x02, 0x00);

			Write4B(response, GA_T::INVENTORY_ID, 994);
			Write4B(response, GA_T::EFFECT_GROUP_ID, 0);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);

			append(response, 0x04, 0x00);

			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, 994);
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 386); // → equip point 10 (morale slot)


	send_response(response);
}

void TcpSession::send_map_randomsm_settings_response(std::vector<std::string> names)
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MAP_RANDOMSM_SETTINGS;
	uint16_t item_count = 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
	append(response, names.size(), 0x00);

	for (auto name : names) {
		append(response, 0x01, 0x00);  // inner item count
		WriteString(response, GA_T::NAME, name);
	}

	send_response(response);
}

void TcpSession::send_get_loot_table_items_by_id_filtered_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
	append(response, 0x01, 0x00);        // count elements
	{
		append(response, 0x06, 0x00);  // inner item count

		Write4B(response, GA_T::LOOT_TABLE_ITEM_ID, 3470);
		Write4B(response, GA_T::ITEM_ID, 2991);
		Write4B(response, GA_T::CREATED_ITEM_ID, 2991);
		Write4B(response, GA_T::AUTO_CREATE_FLAG, 122319617);
		Write4B(response, GA_T::MAX_QUALITY_VALUE_ID, 1165);
		Write4B(response, GA_T::SORT_ORDER, 0);
	}

	append(response, GA_T::DATA_SET_PRICES & 0xFF, GA_T::DATA_SET_PRICES >> 8);        // type 010C
	append(response, 0x01, 0x00);        // count elements
	{
		append(response, 0x04, 0x00);  // inner item count

		Write4B(response, GA_T::LOOT_TABLE_ITEM_ID, 3470);
		Write4B(response, GA_T::CURRENT_PRICE, 0);
		Write4B(response, GA_T::CURRENT_PRICE, 0);
		Write4B(response, GA_T::CURRENCY_TYPE_VALUE_ID, 1603);
	}

	send_response(response);
}

void TcpSession::send_player_skills_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_PLAYER_SKILLS;
	uint16_t item_count = 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::LAST_RESPEC_DATETIME, 0x1);

	send_response(response);

}

void TcpSession::send_agency_get_roster_response()
{
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::AGENCY_GET_ROSTER;
		uint16_t item_count = 1;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		Write4B(response, GA_T::AGENCY_ID, 0x1);

		send_response(response);
	}

void TcpSession::send_start_listen_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_START_LISTEN;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9001);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_player_update_client_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::PLAYER_UPDATE_CLIENT;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::SYS_SITE_ID, 0x2);
	Write4B(response, GA_T::SYS_AVA_TEAMS_OK, 0x1);

	send_response(response);
}

void TcpSession::send_update_new_mail_count_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::UPDATE_NEW_MAIL_COUNT;
	uint16_t item_count = 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::NEW_MAIL_COUNT, 0x0);

	send_response(response);
}

void TcpSession::send_add_player_character_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::ADD_PLAYER_CHARACTER;
	uint16_t item_count = 5;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);

	send_response(response);
}

void TcpSession::send_select_character_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_SELECT_CHARACTER;
	uint16_t item_count = 9;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Write4B(response, GA_T::CALLER_ID, 0x0);
	Write4B(response, GA_T::ERROR_CODE, 0x0);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	Write4B(response, GA_T::CHARACTER_ID, 0x1);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
	WriteString(response, GA_T::CLASS_MSG_ID, "22976");

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));

	WriteIP(response, GA_T::CHAT_NET_ADDR, Config::GetIpChar(), 9001);

	send_response(response);
}

void TcpSession::send_change_map_prep_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::CHANGE_MAP_PREP;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9001);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_change_map_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHANGE_MAP;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9001);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_match_launch_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_LAUNCH_MAP_INSTANCE;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9001);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_go_play_tutorial_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 10;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	// Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	// Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
	WriteIP(response, GA_T::HOST_NET_ADDR, Config::GetIpChar(), Config::GetPort());
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	WriteString(response, GA_T::MAP_FILENAME, "Inception_ALL");
	WriteString(response, GA_T::PARAMETERS, "?Game=TgGame.TgGame_Mission");
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 22845);
	// Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 36021);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, 5289);
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	WriteIP(response, GA_T::CHAT_NET_ADDR, Config::GetIpChar(), 9001);

	send_response(response);
}

void TcpSession::send_marshal_channel_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MARSHAL_CHANNEL;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));
	Write4B(response, GA_T::VERSION, 4869);
	WriteString(response, GA_T::TEXT_VALUE, player_name);

	send_response(response);
}

void TcpSession::send_go_play_response()
{

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 12;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteIP(response, GA_T::HOST_NET_ADDR, Config::GetIpChar(), Config::GetPort());
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	WriteString(response, GA_T::MAP_FILENAME, Config::GetMapNameChar());
	WriteString(response, GA_T::PARAMETERS, Config::GetMapParamsChar());
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 22845);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, 5716);
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	WriteIP(response, GA_T::CHAT_NET_ADDR, Config::GetIpChar(), 9001);

	Write4B(response, GA_T::CLASS_MSG_ID, 22976);

	send_response(response);


}

void TcpSession::send_instance_ready_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MATCH_LAUNCH_TIMEOUT;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::CALLER_ID, 0x0);
	Write4B(response, GA_T::MSG_ID, 33528);

	send_response(response);
}


void TcpSession::send_character_list_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response,	GA_T::PLAYER_NAME, player_name.c_str());

	// ==== arrayofenumblockarrays (010C) ====
	// append(response, 0x0C, 0x01);        // type 010C
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
	append(response, 0x04, 0x00);        // count elements

	// ASSAULT
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0x13);

	// MEDIC
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x0000f69c);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_MEDIC);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Commonwealth HQ");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0xB);

	// RECON
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_RECON);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0x11);

	// ROBO
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ROBOTIC);
	Write4B(response, GA_T::HEAD_ASM_ID,GA_G::GA_G::HEAD_ASM_ID_TROLL);// 0x645);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "alsdfslfjlj");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0x12);

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, 0x00, 0x00);        // count elements

	send_response(response);
}

void TcpSession::send_character_list_queue_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MIN_WAIT_SEC, 0x04567);
	Write4B(response, GA_T::ACTIVE_POSITION, 0x11);
	Write4B(response, GA_T::ENTRY_NUMBER, 0x19);

	send_response(response);

}


void TcpSession::send_login_response()
{
	std::vector<uint8_t> response;

	// handled in disassembly here:
	// 0x10915910
	// -> 0x10923ae0

	uint16_t packet_type = GA_U::GSC_USER_LOGIN_RESPONSE;
	uint16_t item_count = 9;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Write4B(response,		GA_T::PLAYER_ID, 0x0017E6D5);
	Write1B(response,		GA_T::SUCCESS, 1);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	Write4B(response,		GA_T::GAME_BITS, 0x0000000F);
	Write2B(response,		GA_T::NET_ACCESS_FLAGS, 0xF3F8);
	Write4B(response,		GA_T::AFK_TIMEOUT_SEC, 0x00000384);
	WriteNBytes(response,	GA_T::DISPLAY_EULA_FLAG, {0x01, 0x00, 0x6E});
	WriteString(response,	GA_T::PLAYER_NAME, player_name.c_str());
	// Write4B(response,		GA_T::PLAYER_ID, 0x001);
	// Write4B(response,		GA_U::REGISTER_REMOTE, 0x00000000);
	// Write4B(response,		GA_T::MSG_ID, 0x00004949);
	// WriteNBytes(response,	GA_T::BANNED_UNTIL_DATETIME, std::vector<uint8_t>(8));
	// Write4B(response,		GA_T::CONNECTION_HANDLE, 0x00008BCC);

	// ==== arrayofenumblockarrays (010C) ====
	// append(response, 0x0C, 0x01);        // type 010C
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
	append(response, 0x01, 0x00);        // count elements

	append(response, 0x02, 0x00);  // inner item count
	Write4B(response, GA_T::SYS_SITE_ID, 0x00000002);
	Write4B(response, GA_T::NAME_MSG_ID, 0x0000A20B);

	// append(response, 0x02, 0x00);  // inner item count
	// Write4B(response, GA_T::SYS_SITE_ID, 0x00000001); // NA
	// Write4B(response, GA_T::NAME_MSG_ID, 0x0000A26F);

	// append(response, 0x02, 0x00);  // inner item count
	// Write4B(response, GA_T::SYS_SITE_ID, 0x00000002); // EU
	// Write4B(response, GA_T::NAME_MSG_ID, 0x0000A20B);
	//
	// append(response, 0x02, 0x00);  // inner item count
	// Write4B(response, GA_T::SYS_SITE_ID, 0x00000004); // NA West
	// Write4B(response, GA_T::NAME_MSG_ID, 0x0000CFAC);

	append(response, GA_T::DATA_SET_MAP_CONFIGS & 0xFF, GA_T::DATA_SET_MAP_CONFIGS >> 8);    // type 0170
	append(response, 0x01, 0x00);    // 1 item

	append(response, 0x02, 0x00);  // inner item count
	Write4B(response, GA_T::MAP_GAME_ID, 0x0000046B);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

