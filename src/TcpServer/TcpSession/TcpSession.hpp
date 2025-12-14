#pragma once

#include "src/pch.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/TcpServer/TcpEvents/TcpEvents.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"

class TcpSession : public std::enable_shared_from_this<TcpSession> {
public:
    TcpSession(asio::ip::tcp::socket socket)
        : socket_(std::move(socket)) {
        data_.resize(1024);
		socket_.set_option(asio::ip::tcp::no_delay(true));
    }

    void start() {
        do_read();
    }

private:
	asio::ip::tcp::socket socket_;
    std::vector<uint8_t> data_;

    void append(std::vector<uint8_t>& buffer, auto&&... bytes) {
        (buffer.push_back(bytes), ...);
    }

    void WriteTLV(std::vector<uint8_t>& buffer, uint16_t type, const uint8_t* value, size_t len) {
        append(buffer, type & 0xFF, type >> 8);
        buffer.insert(buffer.end(), value, value + len);
    }

    void Write1B(std::vector<uint8_t>& buffer, uint16_t type, uint8_t val) {
        uint8_t buf[] = {
            static_cast<uint8_t>(type & 0xFF),
            static_cast<uint8_t>(type >> 8),
            val
        };
        buffer.insert(buffer.end(), buf, buf + 3);
    }

    void Write2B(std::vector<uint8_t>& buffer, uint16_t type, uint16_t val) {
        uint8_t buf[] = {
            static_cast<uint8_t>(type & 0xFF), static_cast<uint8_t>(type >> 8),
            static_cast<uint8_t>(val & 0xFF), static_cast<uint8_t>(val >> 8)
        };
        buffer.insert(buffer.end(), buf, buf + 4);
    }

    void Write4B(std::vector<uint8_t>& buffer, uint16_t type, uint32_t val) {
        append(buffer, type & 0xFF, type >> 8);
        append(buffer, val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF);
    }

    void WriteString(std::vector<uint8_t>& buffer, uint16_t type, const std::string& str) {
        append(buffer, type & 0xFF, type >> 8);
        append(buffer, static_cast<uint8_t>(str.size()), 0x00);
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

	void WriteWideString(std::vector<uint8_t>& buffer, uint16_t type, const std::wstring& str)
	{
		append(buffer, type & 0xFF, type >> 8);

		uint16_t len = static_cast<uint16_t>(str.size());
		append(buffer, len & 0xFF, len >> 8);

		for (wchar_t ch : str) {
			append(buffer, ch & 0xFF, (ch >> 8) & 0xFF);
		}
	}

    void WriteNBytes(std::vector<uint8_t>& buffer, uint16_t type, const std::vector<uint8_t>& bytes) {
        append(buffer, type & 0xFF, type >> 8);
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    }

	// void WriteFloat(std::vector<uint8_t>& buffer, uint16_t type, float val) {
	// 	append(buffer, type & 0xFF, type >> 8);
	// 	buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&val), reinterpret_cast<uint8_t*>(&val) + sizeof(float));
	// }

	void WriteFloat(std::vector<uint8_t>& buffer, uint16_t type, float val)
	{
		append(buffer, type & 0xFF, type >> 8);

		// static_assert(sizeof(float) == 4, "Unexpected float size");

		uint32_t raw;
		std::memcpy(&raw, &val, sizeof(raw));

		append(buffer,
		 raw & 0xFF,
		 (raw >> 8) & 0xFF,
		 (raw >> 16) & 0xFF,
		 (raw >> 24) & 0xFF
		 );
	}

	void WriteDouble(std::vector<uint8_t>& buffer, uint16_t type, double val)
	{
		append(buffer, type & 0xFF, type >> 8);

		// static_assert(sizeof(double) == 8, "Unexpected double size");

		uint64_t raw;
		std::memcpy(&raw, &val, sizeof(raw));

		append(buffer,
		 raw & 0xFF,
		 (raw >> 8) & 0xFF,
		 (raw >> 16) & 0xFF,
		 (raw >> 24) & 0xFF,
		 (raw >> 32) & 0xFF,
		 (raw >> 40) & 0xFF,
		 (raw >> 48) & 0xFF,
		 (raw >> 56) & 0xFF
		 );
	}

	void WriteIP(std::vector<uint8_t>& response, uint16_t type, const std::string& ip, uint16_t port) {
		response.push_back(type & 0xFF);       // low byte
		response.push_back(type >> 8);         // high byte

		response.push_back(0x02);              // Type ID
		response.push_back(0x00);              // Subtype / reserved

		// Write port in big-endian
		response.push_back(port >> 8);         // High byte
		response.push_back(port & 0xFF);       // Low byte

		// Write IP as 4 bytes
		std::istringstream iss(ip);
		std::string token;
		while (std::getline(iss, token, '.')) {
			response.push_back(static_cast<uint8_t>(std::stoi(token)));
		}
	}

    void PrependLengthPrefix(std::vector<uint8_t>& buffer) {
        uint16_t len = static_cast<uint16_t>(buffer.size());
        buffer.insert(buffer.begin(), {static_cast<uint8_t>(len & 0xFF), static_cast<uint8_t>(len >> 8)});
    }

	void send_response(std::vector<uint8_t>& buffer) {
		std::vector<uint8_t> response;

		// size_t max_chunk_size = 1450; // 1024; // match python max_message_size
		size_t max_chunk_size = 99999; // 1024; // match python max_message_size
		size_t offset = 0;

		while (offset < buffer.size()) {
			size_t remaining = buffer.size() - offset;
			size_t chunk_size = std::min(remaining, max_chunk_size);

			uint16_t length_prefix = static_cast<uint16_t>(chunk_size);
			response.push_back(length_prefix & 0xFF);
			response.push_back(length_prefix >> 8);

			response.insert(response.end(), buffer.begin() + offset, buffer.begin() + offset + chunk_size);

			offset += chunk_size;
		}

		std::error_code ec;
		std::size_t sent = asio::write(socket_, asio::buffer(response), ec);

		// add current time to log


		if (!ec) {
			// LogToFile("C:\\tcplog.txt", "[TCP] Sent %zu bytes as full login response (sync write).", sent);
		} else {
			// LogToFile("C:\\tcplog.txt", "[TCP] Write failed: %s", ec.message().c_str());
		}
	}

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            asio::buffer(data_),
            [this, self](std::error_code ec, std::size_t length) {
                if (!ec) {
                    // LogToFile("C:\\tcplog.txt", "[TCP] Received %zu bytes", length);
                    handle_packet(data_.data(), length);
                    do_read();
                }
            });
    }


    void handle_packet(const uint8_t* data, size_t length) {
        if (length < 6) {
			Logger::Log("tcp", "[%s] Packet too short\n", Logger::GetTime());
            return;
        }

        const uint8_t* p = data + 2;
        uint16_t packet_type = p[0] | (p[1] << 8);
        uint16_t item_count  = p[2] | (p[3] << 8);

        // LogToFile("C:\\tcplog.txt", "Packet type: %04X, item count: %d", packet_type, item_count);

		switch (packet_type) {
			case GA_U::GSC_USER_LOGIN:
				Logger::Log("tcp", "[%s] Received: GSC_USER_LOGIN [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
				send_login_response();
				break;
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
			case GA_U::RELAY_LOG:
				// keepalive
				
				if (GTcpEvents.size() > 0) {
					TcpEvent event = GTcpEvents[0];
					if (event.Type == 1) {
						Logger::Log("tcp", "[%s] Received: RELAY_LOG [0x%04X], item count: %d, sending random SM settings\n", Logger::GetTime(), packet_type, item_count);
						if (event.Names.size() > 0) {
							send_map_randomsm_settings_response(event.Names);
						}
						GTcpEvents.erase(GTcpEvents.begin());
					}

					// 	TcpEvent next;
					// 	next.Type = 3;
					// 	GTcpEvents.push_back(next);
					// } else if (event.Type == 3) {
					// 	Logger::Log("tcp", "[%s] Received: RELAY_LOG [0x%04X], item count: %d, sending character inventory\n", Logger::GetTime(), packet_type, item_count);
					// 	send_map_randomsm_settings_response();
					//
					// 	GTcpEvents.erase(GTcpEvents.begin());
				}

				break;
			case GA_U::GSC_SELECT_CHARACTER:
				Logger::Log("tcp", "[%s] Received: GSC_SELECT_CHARACTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
				send_select_character_response();
				Sleep(1000);
				send_go_play_response();
				Sleep(1000);
				send_inventory_response(998);
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
			case GA_U::PLAYER_LOGOFF:
				Logger::Log("tcp", "[%s] Received: PLAYER_LOGOFF [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
				break;
			case GA_U::ADD_PLAYER_CHARACTER:
				Logger::Log("tcp", "[%s] Received: ADD_PLAYER_CHARACTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
				send_go_play_tutorial_response();
				// send_add_player_character_response();
				// todo
				break;
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
				// send_inventory_response(998); // test
				// send_get_ticket_info_response();
				// send_instance_ready_response();
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

	void send_map_randomsm_settings_response(std::vector<std::string> names) {
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

	void send_get_loot_table_items_by_id_filtered_response() {
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

	void send_inventory_response(int nPawnId);

	// void send_map_randomsm_settings_response() {
	// 	std::vector<uint8_t> response;
	// 	uint16_t packet_type = GA_U::MAP_RANDOMSM_SETTINGS;
	// 	uint16_t item_count = 1;
	//
	// 	append(response, packet_type & 0xFF, packet_type >> 8);
	// 	append(response, item_count & 0xFF, item_count >> 8);
	//
	// 	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
	//
	//
	// 	send_response(response);
	// }


	void send_character_inventory_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::SEND_CHARACTER_INVENTORY;
		uint16_t item_count = 3;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);        // count elements
		{
			append(response, 2, 0x00);  // inner item count

			Write4B(response, GA_T::INVENTORY_ID, 0x1);
			Write4B(response, GA_T::EFFECT_GROUP_ID, 0x1);
		}

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);        // count elements
		{
			append(response, 4, 0x00);  // inner item count

			Write4B(response, GA_T::CHARACTER_ID, 373);
			Write4B(response, GA_T::INVENTORY_ID, 0x1);
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 0x1);
		}

		append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
		append(response, 0x01, 0x00);        // count elements
		{
			append(response, 9, 0x00);  // inner item count

			Write4B(response, GA_T::ITEM_ID, 0x1);
			Write4B(response, GA_T::INVENTORY_ID, 0x1);
			Write4B(response, GA_T::BLUEPRINT_ID, 0x1);
			Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 0x1);
			Write4B(response, GA_T::DURABILITY, 0x1);
			WriteFloat(response, GA_T::ACQUIRE_DATETIME, 0x1);
			Write4B(response, GA_T::BOUND_FLAG, 0x1);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x1);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		}

		send_response(response);
	}


	void send_player_skills_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::SEND_PLAYER_SKILLS;
		uint16_t item_count = 1;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		Write4B(response, GA_T::LAST_RESPEC_DATETIME, 0x1);

		send_response(response);

	}

	void send_agency_get_roster_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::AGENCY_GET_ROSTER;
		uint16_t item_count = 1;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		Write4B(response, GA_T::AGENCY_ID, 0x1);

		send_response(response);
	}


	void send_start_listen_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GSC_START_LISTEN;
		uint16_t item_count = 15;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

		Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9000);
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

	void send_player_update_client_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::PLAYER_UPDATE_CLIENT;
		uint16_t item_count = 2;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		Write4B(response, GA_T::SYS_SITE_ID, 0x2);
		Write4B(response, GA_T::SYS_AVA_TEAMS_OK, 0x1);

		send_response(response);
	}

	void send_update_new_mail_count_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::UPDATE_NEW_MAIL_COUNT;
		uint16_t item_count = 1;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		Write4B(response, GA_T::NEW_MAIL_COUNT, 0x1);

		send_response(response);
	}

	void send_add_player_character_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::ADD_PLAYER_CHARACTER;
		uint16_t item_count = 5;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

		Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);

		send_response(response);
	}

	void send_select_character_response() {

		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GSC_SELECT_CHARACTER;
		uint16_t item_count = 9;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		Write4B(response, GA_T::CALLER_ID, 0x0);
		Write4B(response, GA_T::ERROR_CODE, 0x0);
		Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		Write4B(response, GA_T::TASK_FORCE, 0x1);
		Write4B(response, GA_T::CHARACTER_ID, 0x1);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
		Write4B(response, GA_T::CLASS_MSG_ID, 22976);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");

		WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9001);

		// WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9000);
		// WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));


		// WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
		// WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
		// Write2B(response, GA_T::PARAMETERS, 0x0);
		// Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
		// Write1B(response, GA_T::NON_COMBAT, 0x0);
		// Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
		// Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

		send_response(response);
	}

	void send_change_map_prep_response() {

		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::CHANGE_MAP_PREP;
		uint16_t item_count = 15;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

		Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9000);
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

	void send_change_map_response() {

		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GSC_CHANGE_MAP;
		uint16_t item_count = 15;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

		Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9000);
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

	void send_match_launch_response() {

		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GSC_LAUNCH_MAP_INSTANCE;
		uint16_t item_count = 15;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);

		Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9000);
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


	void send_go_play_tutorial_response() {

		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GSC_GO_PLAY;
		uint16_t item_count = 10;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");
		// Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		// Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
		WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
		WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));
		WriteString(response, GA_T::MAP_FILENAME, "Inception_ALL");
		WriteString(response, GA_T::PARAMETERS, "?Game=TgGame.TgGame_Mission");
		Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 22845);
		// Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 36021);
		Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
		Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
		Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, 5289);
		Write4B(response, GA_T::TASK_FORCE, 0x1);
		// WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9000);

		// Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		// Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		//
		// Write2B(response, GA_T::PARAMETERS, 0x0);
		// Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
		// Write1B(response, GA_T::NON_COMBAT, 0x0);
		// Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
		// Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

		send_response(response);

		// Write4B(response, GA_T::MAP_TYPE, 0x416); // 0x416 has some special meaning
		// Write4B(response, GA_T::MAP_TYPE, 0x1);

		// WriteString(response,	GA_T::PLAYER_NAME, "Zaxik");
		// Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		// Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
		// Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		// Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);


		// Write4B(response, GA_T::ERROR_CODE, 0x0);
		// WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
		// // WriteIP(response, GA_T::HOST_NET_ADDR, "0.0.0.0", 9002);
		// // WriteIP(response, GA_T::HOST_NET_ADDR, "192.168.8.116", 9002);
		// WriteString(response, GA_T::GAME_CLASS_NAME, "TgGame.TgGame_PointRotation");
		// WriteString(response, GA_T::MAP_NAME, "Rot_Redistribution05");
		// WriteString(response, GA_T::MAP_EXE_NAME, "Rot_Redistribution05.ut3");
		// Write4B(response, GA_T::ACCOUNT_ID, 0x2AC950);
		// Write4B(response, GA_T::PLAYER_ID, 0x2AC950);

		// WriteIP(response, GA_T::HOST_IP, "192.168.8.116", 13000);
		// WriteString(response, GA_T::HOST_GSC, "192.168.8.116:9002");
		// WriteString(response, GA_T::HOST_CHAT, "192.168.8.116:9002");
		// WriteString(response, GA_T::HOST_AUCTION, "192.168.8.116:9002");
		// WriteString(response, GA_T::HOST_WORLD_MGR, "192.168.8.116:9002");
		// Write4B(response, GA_T::HOST_PORT, 13500);
		// WriteIP(response, GA_T::PUBLIC_NET_ADDR, "192.168.8.116", 13500);
		// Write4B(response, GA_T::PUBLIC_PORT, 13500);
		// WriteString(response, GA_T::PUBLIC_IP, "192.168.8.116:13500");
		// Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
		// Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::CHARACTER_PROFILE_ID, 0x1);
		// Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
		// Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
		// Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		// Write4B(response, GA_T::HEAD_ASM_ID, 0x645);
		// Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
		// Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
		// WriteString(response, GA_T::URI_NAME, "127.0.0.1:13000/Rot_Redistribution05?game=TgGame.TgGame_PointRotation");
// 107.150.130.77:9002/DomeCity_P_VER_3.ut3
		// WriteString(response, GA_T::CHANNEL_URI, "127.0.0.1:13000/Rot_Redistribution05?game=TgGame.TgGame_PointRotation");
		// Write4B(response, GA_T::XP_VALUE, 0x1F4);
		// Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0xB);
		// Write4B(response, GA_T::LEVEL, 0x32);
		// Write4B(response, GA_T::CHANNEL_ID, 0x2AC950);
		// Write4B(response, GA_T::CHANNEL_TYPE, 0x1);
		// WriteIP(response, GA_T::CLIENT_IP, "127.0.0.1", 0);
		// Write4B(response, GA_T::CLIENT_HANDLE_ID, 0x2AC950);
		// Write4B(response, GA_T::CONNECTION_HANDLE, 0x2AC950);
		// Write4B(response, GA_T::GOTO_MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::MATCH_ID, 0x1);
		// Write4B(response, GA_T::MISSION_ID, 0x1);
		// Write4B(response, GA_T::MISSION_INST_ID, 0x1);
		// Write4B(response, GA_T::MODE_ID, 0x1);
		// Write4B(response, GA_T::SERVER_ID, 0x1);
		// Write4B(response, GA_T::SERVER_TYPE, 0x1);
		// Write1B(response, GA_T::SUCCESS, 1);
		// Write4B(response, GA_T::LOGIN_SESSION_ID, 0x1);
		// Write4B(response, GA_U::GSC_START_LISTEN, 1);
		// Write4B(response, GA_U::MAP_COMMIT, 1);
		// Write4B(response, GA_U::LISTEN_PLAYER, 1);
		// Write4B(response, GA_U::MATCH_JOIN, 1);
		// Write4B(response, GA_U::SEND_ALL_TO_MAP, 1);


		// LOGIN_SESSION_ID = 0x0600, // Type: 3   Name:                              flags: 0x0556 TYPE_TCP_UINT32

		// SESSION_GUID = 0x0473, // Type: 9   Name:                                  flags: 0x03F7 TYPE_TCP_UUID_16_BYTES
// GOTO_MAP_GAME_ID

		// CLIENT_HANDLE_ID = 0x00C7, // Type: 3   Name:                              flags: 0x00B5 TYPE_TCP_UINT32
		// CLIENT_IP = 0x00C8, // Type: 3   Name:                                     flags: 0x00B6 TYPE_TCP_UINT32
		// CHANNEL_ID = 0x00B2, // Type: 3   Name:                                    flags: 0x009E TYPE_TCP_UINT32
		// CHANNEL_TYPE = 0x00B3, // Type: 3   Name:                                  flags: 0x05B1 TYPE_TCP_UINT32
		// CHANNEL_URI = 0x00B4, // Type: 0   Name:                                   flags: 0x00A4 TYPE_TCP_WCHAR_STR

// 1291
		// LogToFile("C:\\tcplog.txt", "GSC_SELECT_CHARACTER response: ");
		// for (int i = 0; i < response.size(); i++) {
		// 	LogToFileInline("C:\\tcplog.txt", "%02X", response[i]);
		// }


		// bInitServerNextTick = true;
		//02 00 23 28 7f 00 00 01

    // @handles(packet=a0039)  # UDP: GSC_SELECT_CHARACTER
    // def handle_a0039(self, request):
    //     self.player.send(a0039().set([
    //         m0241(),  # ERROR_CODE
    //         m02a9().set(0x00000497),  # HOME_MAP_GAME_ID
    //         m00b5().set(0x002ac950),  # CHARACTER_ID
    //         m03c5().set(self.player.display_name),  # PLAYER_NAME
    //         m0327(),  # MAP_NAME_MSG_ID
    //         m03df().set(0x00000237),  # PROFILE_ID
    //         m00c2().set("22976"),  # CLASS_MSG_ID
    //         m0303(),  # LEVEL
    //         m0485(),  # SKILL_RATING
    //         m001f(),  # AGENCY_ID
    //         m0029(),  # ALLIANCE_ID
    //         m0388(),  # OFFLINE_FLAG
    //         m00bf().set(hexparse('02 00 23 29 7f 00 00 01')),  # CHAT_NET_ADDR
    //         # m02b2().set(IPv4Address('127.0.0.1'), 13000),  # HOST_NET_ADDR
    //         m02b2().set(IPv4Address('127.0.0.1'), 13000),  # HOST_NET_ADDR
    //         # m02b2().set(hexparse('02 00 23 28 7f 00 00 01')),  # HOST_NET_ADDR
    //         # m0322().set(0x02A9),  # MAP_GAME_ID
    //         m0322().set(0x050B),  # MAP_GAME_ID
    //         m0325().set(0x1),  # MAP_INSTANCE_ID
    //         m0271().set("TgGame.TgGame_Ticket"),  # GAME_CLASS_NAME
    //     ]))

	}

	void send_marshal_channel_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::MARSHAL_CHANNEL;
		uint16_t item_count = 3;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));
		Write4B(response, GA_T::VERSION, 4869);
		WriteString(response, GA_T::TEXT_VALUE, "Zaxik");

		send_response(response);
	}

	void send_go_play_response() {

		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GSC_GO_PLAY;
		uint16_t item_count = 12;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response, GA_T::PLAYER_NAME, "Zaxik");
		// Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		// Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
		// WriteIP(response, GA_T::HOST_NET_ADDR, "77.237.240.162", 9002);
		WriteIP(response, GA_T::HOST_NET_ADDR, Config::GetIpChar(), Config::GetPort());
		WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));
		WriteString(response, GA_T::MAP_FILENAME, Config::GetMapNameChar());
		WriteString(response, GA_T::PARAMETERS, Config::GetMapParamsChar());
		Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 22845);
		// Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 36021);
		Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
		Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
		Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, 5716);
		Write4B(response, GA_T::TASK_FORCE, 0x1);
		WriteIP(response, GA_T::CHAT_NET_ADDR, "127.0.0.1", 9001);

		Write4B(response, GA_T::CLASS_MSG_ID, 22976);
		// Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		//
		// Write2B(response, GA_T::PARAMETERS, 0x0);
		// Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
		// Write1B(response, GA_T::NON_COMBAT, 0x0);
		// Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
		// Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

		send_response(response);

		// Write4B(response, GA_T::MAP_TYPE, 0x416); // 0x416 has some special meaning
		// Write4B(response, GA_T::MAP_TYPE, 0x1);

		// WriteString(response,	GA_T::PLAYER_NAME, "Zaxik");
		// Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		// Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
		// Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
		// Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);


		// Write4B(response, GA_T::ERROR_CODE, 0x0);
		// WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
		// // WriteIP(response, GA_T::HOST_NET_ADDR, "0.0.0.0", 9002);
		// // WriteIP(response, GA_T::HOST_NET_ADDR, "192.168.8.116", 9002);
		// WriteString(response, GA_T::GAME_CLASS_NAME, "TgGame.TgGame_PointRotation");
		// WriteString(response, GA_T::MAP_NAME, "Rot_Redistribution05");
		// WriteString(response, GA_T::MAP_EXE_NAME, "Rot_Redistribution05.ut3");
		// Write4B(response, GA_T::ACCOUNT_ID, 0x2AC950);
		// Write4B(response, GA_T::PLAYER_ID, 0x2AC950);

		// WriteIP(response, GA_T::HOST_IP, "192.168.8.116", 13000);
		// WriteString(response, GA_T::HOST_GSC, "192.168.8.116:9002");
		// WriteString(response, GA_T::HOST_CHAT, "192.168.8.116:9002");
		// WriteString(response, GA_T::HOST_AUCTION, "192.168.8.116:9002");
		// WriteString(response, GA_T::HOST_WORLD_MGR, "192.168.8.116:9002");
		// Write4B(response, GA_T::HOST_PORT, 13500);
		// WriteIP(response, GA_T::PUBLIC_NET_ADDR, "192.168.8.116", 13500);
		// Write4B(response, GA_T::PUBLIC_PORT, 13500);
		// WriteString(response, GA_T::PUBLIC_IP, "192.168.8.116:13500");
		// Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
		// Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::CHARACTER_PROFILE_ID, 0x1);
		// Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
		// Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
		// Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		// Write4B(response, GA_T::HEAD_ASM_ID, 0x645);
		// Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
		// Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
		// WriteString(response, GA_T::URI_NAME, "127.0.0.1:13000/Rot_Redistribution05?game=TgGame.TgGame_PointRotation");
// 107.150.130.77:9002/DomeCity_P_VER_3.ut3
		// WriteString(response, GA_T::CHANNEL_URI, "127.0.0.1:13000/Rot_Redistribution05?game=TgGame.TgGame_PointRotation");
		// Write4B(response, GA_T::XP_VALUE, 0x1F4);
		// Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0xB);
		// Write4B(response, GA_T::LEVEL, 0x32);
		// Write4B(response, GA_T::CHANNEL_ID, 0x2AC950);
		// Write4B(response, GA_T::CHANNEL_TYPE, 0x1);
		// WriteIP(response, GA_T::CLIENT_IP, "127.0.0.1", 0);
		// Write4B(response, GA_T::CLIENT_HANDLE_ID, 0x2AC950);
		// Write4B(response, GA_T::CONNECTION_HANDLE, 0x2AC950);
		// Write4B(response, GA_T::GOTO_MAP_GAME_ID, 0x050B);
		// Write4B(response, GA_T::MATCH_ID, 0x1);
		// Write4B(response, GA_T::MISSION_ID, 0x1);
		// Write4B(response, GA_T::MISSION_INST_ID, 0x1);
		// Write4B(response, GA_T::MODE_ID, 0x1);
		// Write4B(response, GA_T::SERVER_ID, 0x1);
		// Write4B(response, GA_T::SERVER_TYPE, 0x1);
		// Write1B(response, GA_T::SUCCESS, 1);
		// Write4B(response, GA_T::LOGIN_SESSION_ID, 0x1);
		// Write4B(response, GA_U::GSC_START_LISTEN, 1);
		// Write4B(response, GA_U::MAP_COMMIT, 1);
		// Write4B(response, GA_U::LISTEN_PLAYER, 1);
		// Write4B(response, GA_U::MATCH_JOIN, 1);
		// Write4B(response, GA_U::SEND_ALL_TO_MAP, 1);


		// LOGIN_SESSION_ID = 0x0600, // Type: 3   Name:                              flags: 0x0556 TYPE_TCP_UINT32

		// SESSION_GUID = 0x0473, // Type: 9   Name:                                  flags: 0x03F7 TYPE_TCP_UUID_16_BYTES
// GOTO_MAP_GAME_ID

		// CLIENT_HANDLE_ID = 0x00C7, // Type: 3   Name:                              flags: 0x00B5 TYPE_TCP_UINT32
		// CLIENT_IP = 0x00C8, // Type: 3   Name:                                     flags: 0x00B6 TYPE_TCP_UINT32
		// CHANNEL_ID = 0x00B2, // Type: 3   Name:                                    flags: 0x009E TYPE_TCP_UINT32
		// CHANNEL_TYPE = 0x00B3, // Type: 3   Name:                                  flags: 0x05B1 TYPE_TCP_UINT32
		// CHANNEL_URI = 0x00B4, // Type: 0   Name:                                   flags: 0x00A4 TYPE_TCP_WCHAR_STR

// 1291
		// LogToFile("C:\\tcplog.txt", "GSC_SELECT_CHARACTER response: ");
		// for (int i = 0; i < response.size(); i++) {
		// 	LogToFileInline("C:\\tcplog.txt", "%02X", response[i]);
		// }


		// bInitServerNextTick = true;
		//02 00 23 28 7f 00 00 01

    // @handles(packet=a0039)  # UDP: GSC_SELECT_CHARACTER
    // def handle_a0039(self, request):
    //     self.player.send(a0039().set([
    //         m0241(),  # ERROR_CODE
    //         m02a9().set(0x00000497),  # HOME_MAP_GAME_ID
    //         m00b5().set(0x002ac950),  # CHARACTER_ID
    //         m03c5().set(self.player.display_name),  # PLAYER_NAME
    //         m0327(),  # MAP_NAME_MSG_ID
    //         m03df().set(0x00000237),  # PROFILE_ID
    //         m00c2().set("22976"),  # CLASS_MSG_ID
    //         m0303(),  # LEVEL
    //         m0485(),  # SKILL_RATING
    //         m001f(),  # AGENCY_ID
    //         m0029(),  # ALLIANCE_ID
    //         m0388(),  # OFFLINE_FLAG
    //         m00bf().set(hexparse('02 00 23 29 7f 00 00 01')),  # CHAT_NET_ADDR
    //         # m02b2().set(IPv4Address('127.0.0.1'), 13000),  # HOST_NET_ADDR
    //         m02b2().set(IPv4Address('127.0.0.1'), 13000),  # HOST_NET_ADDR
    //         # m02b2().set(hexparse('02 00 23 28 7f 00 00 01')),  # HOST_NET_ADDR
    //         # m0322().set(0x02A9),  # MAP_GAME_ID
    //         m0322().set(0x050B),  # MAP_GAME_ID
    //         m0325().set(0x1),  # MAP_INSTANCE_ID
    //         m0271().set("TgGame.TgGame_Ticket"),  # GAME_CLASS_NAME
    //     ]))

	}

	void send_instance_ready_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::MATCH_LAUNCH_TIMEOUT;
		uint16_t item_count = 2;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		Write4B(response, GA_T::CALLER_ID, 0x0);
		Write4B(response, GA_T::MSG_ID, 33528);

		send_response(response);
	}

	void send_get_ticket_info_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GET_TICKET_INFO;
		uint16_t item_count = 4;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		// GA_T::CALLER_ID
		// GA_T::TEAM_LEADER_FLAG
		// GA_T::CURRENT_MATCH_QUEUE_ID
		// GA_T::DATA_SET

		Write4B(response, GA_T::CALLER_ID, 0x0);
		Write1B(response, GA_T::TEAM_LEADER_FLAG, 0x1);
		Write4B(response, GA_T::CURRENT_MATCH_QUEUE_ID, 0x0);

		// ==== arrayofenumblockarrays (010C) ====
		// append(response, 0x0C, 0x01);        // type 010C
		append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
		append(response, 1, 0x00);        // count elements

			append(response, 29, 0x00);  // inner item count
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x1);
			Write4B(response, GA_T::SYS_SITE_ID, 0x1);
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x58C);
			Write4B(response, GA_T::STATUS_MSG_ID, 0x1);
			Write4B(response, GA_T::NAME_MSG_ID, 61298);
			Write4B(response, GA_T::DESC_MSG_ID, 0x1);
			Write4B(response, GA_T::ICON_ID, 0x1);
			Write4B(response, GA_T::PLAYER_COUNT, 0x25);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0xA);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x1);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0xA);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x7);
			Write4B(response, GA_T::LEVEL_MIN, 0x1);
			Write4B(response, GA_T::LEVEL_MAX, 0x999);
			Write4B(response, GA_T::MAP_X, 0x0);
			Write4B(response, GA_T::MAP_Y, 0x0);
			Write1B(response, GA_T::MAP_ACTIVE_FLAG, 0x1);
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x0323);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x0543);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x0310);
			Write1B(response, GA_T::LOCKED_FLAG, 0x1);
			Write1B(response, GA_T::DOUBLE_AGENT_FLAG, 0x1);
			Write4B(response, GA_T::SORT_ORDER, 0x01);
			Write1B(response, GA_T::BONUS_QUEUE_FLAG, 0x1);
			Write4B(response, GA_T::ACCESS_FLAGS, 0x0008);
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x0210);
			Write1B(response, GA_T::ACTIVE_FLAG, 0x1);
			Write4B(response, GA_T::REMAINING_SECONDS, 0x0429);

			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);        // type 010C
			append(response, 0x04, 0x00);        // count elements

			append(response, 0x02, 0x00);		// inner item count
			Write1B(response, GA_T::PLAYER_PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
			Write1B(response, GA_T::PLAYER_PROFILE_COUNT, 0x9);

			append(response, 0x02, 0x00);		// inner item count
			Write1B(response, GA_T::PLAYER_PROFILE_ID, GA_G::GA_G::PROFILE_ID_MEDIC);
			Write1B(response, GA_T::PLAYER_PROFILE_COUNT, 0x8);

			append(response, 0x02, 0x00);		// inner item count
			Write1B(response, GA_T::PLAYER_PROFILE_ID, GA_G::GA_G::PROFILE_ID_RECON);
			Write1B(response, GA_T::PLAYER_PROFILE_COUNT, 0x7);

			append(response, 0x02, 0x00);		// inner item count
			Write1B(response, GA_T::PLAYER_PROFILE_ID, GA_G::GA_G::PROFILE_ID_ROBOTIC);
			Write1B(response, GA_T::PLAYER_PROFILE_COUNT, 0x6);

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



		send_response(response);
	}


	void send_character_list_response() {
		std::vector<uint8_t> response;

		uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
		uint16_t item_count = 3;

		append(response, packet_type & 0xFF, packet_type >> 8);
		append(response, item_count & 0xFF, item_count >> 8);

		WriteString(response,	GA_T::PLAYER_NAME, "Zaxik");

		// ==== arrayofenumblockarrays (010C) ====
		// append(response, 0x0C, 0x01);        // type 010C
		append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);        // type 010C
		append(response, 0x04, 0x00);        // count elements

		// ASSAULT
		append(response, 0x0C, 0x00);  // inner item count
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ASSAULT);
		Write4B(response, GA_T::HEAD_ASM_ID, GA_G::GA_G::HEAD_ASM_ID_TROLL);
		Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
		WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
		Write4B(response, GA_T::XP_VALUE, 0x15F4);
		Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0xB);
		Write4B(response, GA_T::LEVEL, 0x32);

		// MEDIC
		append(response, 0x0C, 0x00);  // inner item count
		Write4B(response, GA_T::CHARACTER_ID, 0x1);
		Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_MEDIC);
		Write4B(response, GA_T::HEAD_ASM_ID, GA_G::GA_G::HEAD_ASM_ID_TROLL);
		Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
		WriteString(response, GA_T::MAP_NAME, "Commonwealth HQ");
		Write4B(response, GA_T::XP_VALUE, 0x27F4);
		Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0xB);
		Write4B(response, GA_T::LEVEL, 0x32);

		// RECON
		append(response, 0x0C, 0x00);  // inner item count
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_RECON);
		Write4B(response, GA_T::HEAD_ASM_ID, GA_G::GA_G::HEAD_ASM_ID_TROLL);
		Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
		WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
		Write4B(response, GA_T::XP_VALUE, 0x39F4);
		Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0xB);
		Write4B(response, GA_T::LEVEL, 0x32);

		// ROBO
		append(response, 0x0C, 0x00);  // inner item count
		Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
		Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::PROFILE_ID, GA_G::GA_G::PROFILE_ID_ROBOTIC);
		Write4B(response, GA_T::HEAD_ASM_ID,GA_G::GA_G::HEAD_ASM_ID_TROLL);// 0x645);
		Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
		WriteString(response, GA_T::MAP_NAME, "alsdfslfjlj");
		Write4B(response, GA_T::XP_VALUE, 0x4AF4);
		Write4B(response, GA_T::SKILL_GROUP_SET_ID, 0xB);
		Write4B(response, GA_T::LEVEL, 0x32);

		append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
		append(response, 0x01, 0x00);        // count elements

		append(response, 0x03, 0x00);  // inner item count

		Write4B(response, GA_T::CHARACTER_ID, 0x1);

		Write4B(response, GA_T::ITEM_ID, 5883);
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 0);

		send_response(response);
	}

	void send_character_list_queue_response() {
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

	void send_login_response() {
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
		WriteNBytes(response,	GA_T::SESSION_GUID, std::vector<uint8_t>(16));
		Write4B(response,		GA_T::GAME_BITS, 0x0000000F);
		Write2B(response,		GA_T::NET_ACCESS_FLAGS, 0xF3F8);
		Write4B(response,		GA_T::AFK_TIMEOUT_SEC, 0x00000384);
		WriteNBytes(response,	GA_T::DISPLAY_EULA_FLAG, {0x01, 0x00, 0x6E});
		WriteString(response,	GA_T::PLAYER_NAME, "Zaxik");
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

	static std::string get_current_datetime() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

		std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&tm, &now_time_t);  // Windows
#else
		localtime_r(&now_time_t, &tm);  // POSIX
#endif

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		return oss.str();
	}

    static void LogToFile(const std::string& path, const char* fmt, ...) {
		FILE* file = fopen(path.c_str(), "a");
		if (!file) return;

		// Print timestamp
		std::string datetime = get_current_datetime();
		fprintf(file, "[%s] ", datetime.c_str());

		// Print formatted message
		va_list args;
		va_start(args, fmt);
		vfprintf(file, fmt, args);
		va_end(args);

		fprintf(file, "\n");
		fclose(file);
    }
};

