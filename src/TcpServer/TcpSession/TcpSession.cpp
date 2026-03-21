#include "src/TcpServer/TcpSession/TcpSession.hpp"
#include "src/Database/Database.hpp"


void TcpSession::handle_packet(const uint8_t* data, size_t length) {
	if (length < 6) {
		Logger::Log("tcp", "[%s] Packet too short\n", Logger::GetTime());
		return;
	}

	const uint8_t* p = data + 2;
	uint16_t packet_type = p[0] | (p[1] << 8);
	uint16_t item_count  = p[2] | (p[3] << 8);

	// LogToFile("C:\\tcplog.txt", "Packet type: %04X, item count: %d", packet_type, item_count);

	switch (packet_type) {
		case GA_U::GSC_USER_LOGIN: {

			PacketView pkt(data + 6, length - 6);
			player_name = pkt.ReadString(GA_T::USER_NAME).value_or("unknown");
			session_guid_ = GenerateSessionGuid();
			ip_address_ = socket_.remote_endpoint().address().to_string() + ":" + std::to_string(socket_.remote_endpoint().port());
			user_id_ = PlayerRegistry::UpsertUser(player_name);

			{
				PlayerInfo info;
				info.session_guid = session_guid_;
				info.player_name = player_name;
				info.player_name_w = CommandLineParser::Utf8ToWide(player_name);
				info.ip_address = ip_address_;
				info.user_id = user_id_;
				PlayerRegistry::Register(info);
			}

			Logger::Log("tcp", "[%s] Received: GSC_USER_LOGIN [0x%04X], name: %s guid: %s ip: %s\n",
			   Logger::GetTime(), packet_type, player_name.c_str(), session_guid_.c_str(), ip_address_.c_str());

			send_login_response();
			break;
		}
		case GA_U::GSC_CHARACTER_LIST:
			Logger::Log("tcp", "[%s] Received: GSC_CHARACTER_LIST [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_character_list_response();
			// send_character_list_queue_response();
			break;
		case GA_U::PLAYER_UPDATE_CLIENT:
			Logger::Log("tcp", "[%s] Received: PLAYER_UPDATE_CLIENT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_player_update_client_response();
			break;
		case GA_U::GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED:
			Logger::Log("tcp", "[%s] Received: GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_get_loot_table_items_by_id_filtered_response();
			break;
		case GA_U::RELAY_LOG: {
			// keepalive — check if a spawn event is pending for this session
			if (!session_guid_.empty()) {
				auto it = GTcpEvents.find(session_guid_);
				if (it != GTcpEvents.end()) {
					TcpEvent& event = it->second;
					if (event.Type == 1) {
						int retries = event.IntValues["retries"];
						int pawnId = (event.Pawn != nullptr) ? event.Pawn->r_nPawnId : 998;
						Logger::Log("tcp", "[%s] Received: RELAY_LOG, player '%s' spawned (pawnId=%d, retry %d)\n",
				  Logger::GetTime(), player_name.c_str(), pawnId, retries);
						send_inventory_response(pawnId);
						if (retries >= 9) {
							GTcpEvents.erase(it);
						} else {
							event.IntValues["retries"] = retries + 1;
						}
					}
				}

				// Drain any queued mid-game inventory-add packets (e.g. beacon pickup).
				auto bit = GBeaconPickupEvents.find(session_guid_);
				if (bit != GBeaconPickupEvents.end() && !bit->second.empty()) {
					for (auto& bev : bit->second) {
						Logger::Log("tcp", "[%s] RELAY_LOG: sending beacon pickup packet pawnId=%d deviceId=%d invId=%d slot=%d\n",
							Logger::GetTime(), bev.nPawnId, bev.nDeviceId, bev.nInventoryId, bev.nEquipSlotValueId);
						send_beacon_pickup_response(bev.nPawnId, bev.nDeviceId, bev.nInventoryId, bev.nEquipSlotValueId);
					}
					GBeaconPickupEvents.erase(bit);
				}

				auto rit = GBeaconRemoveEvents.find(session_guid_);
				if (rit != GBeaconRemoveEvents.end() && !rit->second.empty()) {
					for (auto& rev : rit->second) {
						Logger::Log("tcp", "[%s] RELAY_LOG: sending beacon remove packet pawnId=%d invId=%d\n",
							Logger::GetTime(), rev.nPawnId, rev.nInventoryId);
						send_beacon_remove_response(rev.nPawnId, rev.nInventoryId);
					}
					GBeaconRemoveEvents.erase(rit);
				}

				auto qit = GQuestEvents.find(session_guid_);
				if (qit != GQuestEvents.end() && !qit->second.empty()) {
					for (auto& qev : qit->second) {
						if (qev.action == QuestAction::Abandon) {
							Logger::Log("tcp", "[%s] RELAY_LOG: quest abandon questId=%d charId=%lld\n",
								Logger::GetTime(), qev.nQuestId, selected_character_id_);
							Database::AbandonQuest(selected_character_id_, qev.nQuestId);
							send_quest_abandon_response(qev.nQuestId);
						} else {
							auto status = Database::GetQuestStatus(selected_character_id_, qev.nQuestId);
							Logger::Log("tcp", "[%s] RELAY_LOG: quest request questId=%d charId=%lld status='%s'\n",
								Logger::GetTime(), qev.nQuestId, selected_character_id_, status.c_str());
							if (status == "active") {
								Database::CompleteQuest(selected_character_id_, qev.nQuestId);
								send_quest_complete_response(qev.nQuestId);
							} else if (status.empty()) {
								Database::AcceptQuest(selected_character_id_, qev.nQuestId);
								send_quest_accept_response(qev.nQuestId);
							}
							// status == "complete": already done, ignore
						}
					}
					GQuestEvents.erase(qit);
				}
			}
			break;
		}
		case GA_U::GSC_SELECT_CHARACTER: {
			PacketView pkt(data + 6, length - 6);
			int64_t nCharacterId = pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);

			Logger::Log("tcp", "[%s] Received: GSC_SELECT_CHARACTER [0x%04X] charId=%I64d session_guid=%s\n",
				Logger::GetTime(), packet_type, nCharacterId, session_guid_.c_str());

			LogData(data, length);

			// Look up the requested character from DB and select it.
			if (nCharacterId != 0) {
				auto charInfo = PlayerRegistry::GetCharacterById(nCharacterId);
				if (charInfo && charInfo->user_id == user_id_) {
					selected_character_id_ = charInfo->id;
					selected_profile_id_   = charInfo->profile_id;
					PlayerRegistry::SetSelectedCharacter(session_guid_, charInfo->id, charInfo->profile_id);
					Logger::Log("tcp", "[%s] GSC_SELECT_CHARACTER: selected charId=%I64d profile=%u\n",
						Logger::GetTime(), selected_character_id_, selected_profile_id_);
				} else {
					Logger::Log("tcp", "[%s] GSC_SELECT_CHARACTER: charId=%I64d not found or wrong user (user_id_=%I64d)\n",
						Logger::GetTime(), nCharacterId, user_id_);
				}
			}

			send_select_character_response();
			Sleep(1000);
			send_go_play_response();
			// Sleep(1000);
			// send_character_inventory_response(998);
			// Sleep(1000);
			// send_inventory_response(998);
			// while (!TgGame__LoadGameConfig::bRandomSMSettingsLoaded) {
			// 	Sleep(100);
			// }
			//
			// if (TgGame__LoadGameConfig::m_randomSMSettings.size() > 0) {
			// 	send_map_randomsm_settings_response(TgGame__LoadGameConfig::m_randomSMSettings);
			// }

			// Sleep(1);
			// send_marshal_channel_response();
			break;
		}
		case GA_U::PLAYER_LOGOFF:
			Logger::Log("tcp", "[%s] Received: PLAYER_LOGOFF [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			break;
		case GA_U::ADD_PLAYER_CHARACTER: {
			Logger::Log("tcp", "[%s] Received: ADD_PLAYER_CHARACTER [0x%04X]\n", Logger::GetTime(), packet_type);
			PacketView pkt(data + 6, length - 6);
			uint32_t trainingMapGameId  = pkt.Read4B(GA_T::TRAINING_MAP_GAME_ID).value_or(0);
			uint32_t profileId          = pkt.Read4B(GA_T::PROFILE_ID).value_or(GA_G::GA_G::PROFILE_ID_ASSAULT);
			uint32_t headAsmId          = pkt.Read4B(GA_T::HEAD_ASM_ID).value_or(1605);
			uint32_t genderTypeId       = pkt.Read4B(GA_T::GENDER_TYPE_VALUE_ID).value_or(853);

			// DWORDS payload: 4B length prefix + raw morph bytes — strip prefix before storing.
			std::vector<uint8_t> morphBlob;
			auto dwordsRaw = pkt.ReadNBytes(GA_T::DWORDS);
			if (dwordsRaw && dwordsRaw->size() > 4)
				morphBlob.assign(dwordsRaw->begin() + 4, dwordsRaw->end());

			selected_character_id_ = PlayerRegistry::InsertCharacter(user_id_, profileId, headAsmId, genderTypeId, morphBlob);
			selected_profile_id_   = profileId;

			Logger::Log("tcp", "[%s] ADD_PLAYER_CHARACTER: profile=%u head=%u gender=%u morphBytes=%u charId=%I64d\n",
				Logger::GetTime(), profileId, headAsmId, genderTypeId, (unsigned)morphBlob.size(), selected_character_id_);

			send_add_player_character_response();
			if (trainingMapGameId != 0)
				// send_go_play_tutorial_response();
				send_go_play_response();
			else
				send_go_play_response();
			break;
		}
		case GA_U::DELETE_CHARACTER:
			Logger::Log("tcp", "[%s] Received: DELETE_CHARACTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			// todo
			break;
		case GA_U::UPDATE_NEW_MAIL_COUNT:
			Logger::Log("tcp", "[%s] Received: UPDATE_NEW_MAIL_COUNT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_update_new_mail_count_response();
			break;
		case GA_U::GET_TICKET_INFO:
			Logger::Log("tcp", "[%s] Received: GET_TICKET_INFO [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_get_ticket_info_response();
			break;
		case GA_U::AGENCY_GET_ROSTER:
			Logger::Log("tcp", "[%s] Received: AGENCY_GET_ROSTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_agency_get_roster_response();
			break;
		default:
			Logger::Log("tcp", "[%s] Received unknown packet type: %04X, raw data:\n", Logger::GetTime(), packet_type);
			for (int i = 0; i < length; i++) {
				Logger::Log("tcp", "%02X", data[i]);
			}
			Logger::Log("tcp", "\n");
			break;
	}

}

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

// OLD send_inventory_response (replaced by Inventory::GetEquippedByPawnId loop):
// The old implementation hardcoded 7 medic loadout items (CJP jetpack, Agonizer, specialty 864,
// HealingGrenade 2531, LifeStealer 5800, specialty 2906, HealingBoost 2773) with fixed inventory
// IDs 999-994. Replaced with a data-driven loop so the marshal automatically reflects whatever
// Inventory::Equip was called with during SpawnPlayerCharacter.
//
// void TcpSession::send_inventory_response(int nPawnId) {
//     ... (256 lines of hardcoded items removed) ...
// }

void TcpSession::send_inventory_response(int nPawnId) {
	const auto& items = Inventory::GetEquippedByPawnId(nPawnId);
	if (items.empty()) {
		Logger::Log("tcp", "[TCP] send_inventory_response: no equipped items for pawnId=%d\n", nPawnId);
		return;
	}

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_INVENTORY;
	uint16_t item_count = 2;  // packet header field count (PAWN_ID + DATA_SET)

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	// DATA_SET header: type + item count
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	uint16_t dataSetCount = static_cast<uint16_t>(items.size());
	append(response, dataSetCount & 0xFF, dataSetCount >> 8);

	for (const auto& entry : items) {
		int slotValueId = GA::SlotValueId(entry.slot);

		// 14 fields per item (0x0E)
		append(response, 0x0E, 0x00);

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, entry.deviceId);
		Write4B(response, GA_T::INVENTORY_ID, entry.inventoryId);
		Write4B(response, GA_T::BLUEPRINT_ID, 0);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, entry.quality != 0 ? entry.quality : 1162);
		Write4B(response, GA_T::DURABILITY, 100);
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, entry.inventoryId);  // must match r_nDeviceInstanceId

		// DATA_SET_INVENTORY_STATE (1 entry, 2 fields)
		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, entry.inventoryId);
			Write4B(response, GA_T::EFFECT_GROUP_ID, entry.effectGroupId);

		// DATA_SET_CHARACTER_PROFILES (1 entry, 4 fields)
		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x04, 0x00);
			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, entry.inventoryId);
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, slotValueId);
	}

	send_response(response);
}

void TcpSession::send_beacon_pickup_response(int nPawnId, int nDeviceId, int nInventoryId, int nEquipSlotValueId) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_INVENTORY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);  // 1 item in DATA_SET

		append(response, 0x0E, 0x00);  // 14 fields per item

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, nDeviceId);
		Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
		Write4B(response, GA_T::BLUEPRINT_ID, 0);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162);
		Write4B(response, GA_T::DURABILITY, 100);
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, nInventoryId);

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
			Write4B(response, GA_T::EFFECT_GROUP_ID, 0);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x04, 0x00);
			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, nEquipSlotValueId);

	send_response(response);
}

void TcpSession::send_beacon_remove_response(int nPawnId, int nInventoryId) {
	std::vector<uint8_t> response;

	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);  // 2 top-level items: PAWN_ID + DATA_SET

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);  // 1 item in DATA_SET
		append(response, 0x02, 0x00);  // 2 fields
		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x2);  // 2 = deleted
		Write4B(response, GA_T::INVENTORY_ID, nInventoryId);

	send_response(response);
}

void TcpSession::send_quest_accept_response(int nQuestId) {
	std::vector<uint8_t> response;

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x02, 0x00);  // 2 fields: QUEST_ID + ACCEPT_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		Write1B(response, GA_T::ACCEPT_FLAG, 0x01);

	send_response(response);
}

void TcpSession::send_quest_complete_response(int nQuestId) {
	std::vector<uint8_t> response;

	// Current time as a non-zero 64-bit timestamp — triggers the complete branch in LoadQuests.
	uint64_t ts = static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x03, 0x00);  // 3 fields: QUEST_ID + COMPLETED_DATETIME + COMPLETE_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		// COMPLETED_DATETIME (0x00D8): 8-byte value — non-zero triggers completion branch
		append(response, GA_T::COMPLETED_DATETIME & 0xFF, GA_T::COMPLETED_DATETIME >> 8);
		for (int i = 0; i < 8; i++) append(response, static_cast<uint8_t>((ts >> (i * 8)) & 0xFF));
		Write1B(response, GA_T::COMPLETE_FLAG, 0x01);

	send_response(response);
}

void TcpSession::send_quest_abandon_response(int nQuestId) {
	std::vector<uint8_t> response;

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x02, 0x00);  // 2 fields: QUEST_ID + ABANDON_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		Write1B(response, GA_T::ABANDON_FLAG, 0x01);

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
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	Write4B(response, GA_T::PROFILE_ID, selected_profile_id_);

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
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	Write4B(response, GA_T::PROFILE_ID, selected_profile_id_ != 0 ? selected_profile_id_ : GA_G::GA_G::PROFILE_ID_ASSAULT);
	WriteString(response, GA_T::CLASS_MSG_ID, "22976");

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	Logger::Log("tcp", "[%s] send_select_character_response: charId=%I64d profile=%u session_guid=%s\n",
		Logger::GetTime(), selected_character_id_, selected_profile_id_, session_guid_.c_str());

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
	auto characters = PlayerRegistry::GetCharactersByUserId(user_id_);

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name.c_str());

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, static_cast<uint8_t>(characters.size() & 0xFF),
	                 static_cast<uint8_t>(characters.size() >> 8));

	for (const auto& c : characters) {
		append(response, 0x0B, 0x00);  // inner item count = 11
		Write4B(response, GA_T::CHARACTER_ID,           static_cast<uint32_t>(c.id));
		Write4B(response, GA_T::CHARACTER_LEVEL,        0x32);
		Write4B(response, GA_T::CURRENT_LEVEL,          0x32);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::PROFILE_ID,             c.profile_id);
		Write4B(response, GA_T::HEAD_ASM_ID,            c.head_asm_id);
		Write4B(response, GA_T::HAIR_ASM_ID,            0x85D);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID,   c.gender_type_value_id);
		WriteString(response, GA_T::MAP_NAME,           "Scramble: Tetra Pier");
		Write4B(response, GA_T::XP_VALUE,               0xbbddc);
		Write4B(response, GA_T::SKILL_GROUP_SET_ID,     GetClassConfig(c.profile_id).skillGroupSetId);
	}

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, 0x00, 0x00);  // count elements

	send_response(response);
}

void TcpSession::send_character_list_response_mock()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response,	GA_T::PLAYER_NAME, player_name.c_str());

	// ==== arrayofenumblockarrays (010C) ====
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
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::GA_G::SKILL_GROUP_SET_ID_ASSAULT);

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
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::GA_G::SKILL_GROUP_SET_ID_MEDIC);

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
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::GA_G::SKILL_GROUP_SET_ID_RECON);

	// ROBO
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ROBOTIC);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "alsdfslfjlj");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::GA_G::SKILL_GROUP_SET_ID_ROBOTIC);

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

