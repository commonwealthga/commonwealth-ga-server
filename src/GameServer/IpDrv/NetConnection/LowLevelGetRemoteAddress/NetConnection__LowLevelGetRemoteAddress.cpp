#include "src/pch.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelGetRemoteAddress/NetConnection__LowLevelGetRemoteAddress.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Core/FString/CreateFromWCharT/FString__CreateFromWCharT.hpp"

void NetConnection__LowLevelGetRemoteAddress::Call(UNetConnection* NetConnection, void* edx, void* Out) {
    // param_1 is pointer to the caller-allocated FString memory (3 x 4 bytes)
    // operate with raw pointer arithmetic (no struct)
    uint32_t *pParam = (uint32_t*)Out;

    // mirror original: zero out the incoming FString triple first (like the engine does)
    // param_1[0] = 0; param_1[1] = 0; param_1[2] = 0;
    if (pParam) {
        pParam[0] = 0;
        pParam[1] = 0;
        pParam[2] = 0;
    }

    // Find our stored client info using NetConnection as the key (you used int32 index previously)
    int32_t index = (int32_t)NetConnection;
    auto it = GClientConnectionsData.find(index);
    if (it == GClientConnectionsData.end()) {
        // LogToFile("C:\\mylog.txt", "connection not found");
        return; // keep behavior simple — leave FString zeroed like original started
    }

    // local wchar buffer same size as the original implementation used
    wchar_t local_50[32]; // room for small "IP:port" strings
    // Ensure zero-init
    local_50[0] = L'\0';

    // Source ANSI string stored for this connection
    const char* src = it->second.RemoteAddrString;
    if (!src) {
        // nothing to copy
		FString__CreateFromWCharT::CallOriginal(Out, nullptr, local_50);
        return;
    }

    // Convert ASCII/ANSI src -> UTF-16 in local_50 safely, truncating if necessary.
    // local_50 capacity is 32 wchar_t (including null terminator).
    int maxChars = 31; // leave space for NUL
    int i = 0;
    for (; i < maxChars && src[i] != '\0'; ++i) {
        // map each byte to a wchar (0..255)
        local_50[i] = (wchar_t)(unsigned char)src[i];
    }
    local_50[i] = L'\0';

    // Now call the engine helper to populate the FString properly (engine allocator, etc.)
	FString__CreateFromWCharT::CallOriginal(Out, nullptr, local_50);

    // done; return to caller
    return;
}
