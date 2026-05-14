#pragma once

#include <cstdint>
#include <unordered_map>
#include <chrono>

// Tracks UDP source (IP:port) pairs of UNetConnections we just cleaned up.
//
// Why: after a client disconnects, late in-flight UDP packets from that source
// (acks, replication tail, etc.) can still arrive at the server socket. The
// previous UNetConnection is already gone from ClientConnections, so the
// matching loop in UdpNetDriver__TickDispatch fails to find an owner and
// fabricates a brand-new UNetConnection for the stale packet. That "phantom"
// connection is registered with NetDriver->ClientConnections and the engine
// happily runs its normal output pipeline on it (replication, control bunches,
// etc.) — observed empirically while debugging reconnect kicks.
//
// Stock UE3 has the same scenario in mind and handles it via the
// "new instance of a previous connection, so reject it" path in UnConn.cpp.
// Our reimplemented UdpNetDriver doesn't have that gate, so we replicate it
// by simply refusing to spawn a new connection from any IP:port we just
// cleaned up within the guard window.
namespace RecentlyClosedAddrs {

inline std::unordered_map<uint64_t, std::chrono::steady_clock::time_point>& Map() {
	static std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> g;
	return g;
}

// IP and port are stored in network byte order (the form sockaddr_in carries).
// We don't need to canonicalize — we always compare like-for-like.
inline uint64_t Key(uint32_t ip_be, uint16_t port_be) {
	return (uint64_t)ip_be | ((uint64_t)port_be << 32);
}

inline void Mark(uint32_t ip_be, uint16_t port_be) {
	Map()[Key(ip_be, port_be)] = std::chrono::steady_clock::now();
}

inline bool IsRecent(uint32_t ip_be, uint16_t port_be, double window_seconds) {
	auto& m = Map();
	auto it = m.find(Key(ip_be, port_be));
	if (it == m.end()) return false;
	auto elapsed = std::chrono::duration<double>(
		std::chrono::steady_clock::now() - it->second).count();
	if (elapsed > window_seconds) {
		m.erase(it);
		return false;
	}
	return true;
}

} // namespace RecentlyClosedAddrs
