#pragma once

#include "src/pch.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Constants/TcpFunctions.h"
#include "src/GameServer/Constants/TcpTypes.h"
#include "src/TcpServer/TcpEvents/TcpEvents.hpp"
#include "src/TcpServer/TcpSession/PacketView.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Utils/CommandLineParser/CommandLineParser.hpp"
#include <random>
#include <sstream>
#include <string>

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

	std::string player_name;
	std::string session_guid_;
	std::string ip_address_;

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

	void WriteNBytesRaw(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& bytes) {
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

	static std::string GenerateSessionGuid() {
		std::mt19937 gen(std::random_device{}());
		std::uniform_int_distribution<unsigned int> dist(0, 255);
		uint8_t bytes[16];
		for (auto& b : bytes) b = static_cast<uint8_t>(dist(gen));
		bytes[6] = (bytes[6] & 0x0F) | 0x40;
		bytes[8] = (bytes[8] & 0x3F) | 0x80;
		std::ostringstream oss;
		for (auto b : bytes)
			oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
		return oss.str();
	}

	static std::vector<uint8_t> GuidHexToBytes(const std::string& hex) {
		std::vector<uint8_t> bytes;
		for (size_t i = 0; i + 1 < hex.size(); i += 2)
			bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
		return bytes;
	}


    void PrependLengthPrefix(std::vector<uint8_t>& buffer) {
        uint16_t len = static_cast<uint16_t>(buffer.size());
        buffer.insert(buffer.begin(), {static_cast<uint8_t>(len & 0xFF), static_cast<uint8_t>(len >> 8)});
    }

	void send_response(std::vector<uint8_t>& buffer) {
		std::vector<uint8_t> response;

		size_t max_chunk_size = 1450; // 1024; // match python max_message_size
		// size_t max_chunk_size = 99999; // 1024; // match python max_message_size
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
                    handle_packet(data_.data(), length);
                    do_read();
                } else {
                    if (!session_guid_.empty()) {
                        PlayerRegistry::Unregister(session_guid_);
                    }
                    Logger::Log("tcp", "[TCP] Client disconnected: %s\n", player_name.c_str());
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
			case GA_U::GSC_USER_LOGIN: {

				PacketView pkt(data + 6, length - 6);
				player_name = pkt.ReadString(GA_T::USER_NAME).value_or("unknown");
				session_guid_ = GenerateSessionGuid();
				ip_address_ = socket_.remote_endpoint().address().to_string() + ":" + std::to_string(socket_.remote_endpoint().port());

				{
					PlayerInfo info;
					info.session_guid = session_guid_;
					info.player_name = player_name;
					info.player_name_w = CommandLineParser::Utf8ToWide(player_name);
					info.ip_address = ip_address_;
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
				}
				break;
			}
			case GA_U::GSC_SELECT_CHARACTER:
				Logger::Log("tcp", "[%s] Received: GSC_SELECT_CHARACTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
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

	void send_map_randomsm_settings_response(std::vector<std::string> names);

	void send_get_loot_table_items_by_id_filtered_response();

	void send_inventory_response(int nPawnId);

	void send_character_inventory_response(int nPawnId);

	void send_player_skills_response();

	void send_agency_get_roster_response();

	void send_start_listen_response();

	void send_player_update_client_response();

	void send_update_new_mail_count_response();

	void send_add_player_character_response();

	void send_select_character_response();

	void send_change_map_prep_response();

	void send_change_map_response();

	void send_match_launch_response();

	void send_go_play_tutorial_response();

	void send_marshal_channel_response();

	void send_go_play_response();

	void send_instance_ready_response();

	void send_get_ticket_info_response();

	void send_character_list_response();

	void send_character_list_queue_response();

	void send_login_response();

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

