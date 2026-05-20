#include "src/pch.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelGetRemoteAddress/NetConnection__LowLevelGetRemoteAddress.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Core/FString/CreateFromWCharT/FString__CreateFromWCharT.hpp"
#include "src/Utils/Logger/Logger.hpp"

void* __fastcall NetConnection__LowLevelGetRemoteAddress::Call(UNetConnection* NetConnection, void* edx, void* Out) {
	LogCallBegin();

	uint32_t* pParam = (uint32_t*)Out;
	if (pParam) {
		pParam[0] = 0;
		pParam[1] = 0;
		pParam[2] = 0;
	}

	auto it = GClientConnectionsData.find((int32_t)NetConnection);
	if (it == GClientConnectionsData.end()) {
		Logger::Log(GetLogChannel(), "Connection not found\n");
		LogCallEnd();
		return nullptr;
	}

	const char* src = it->second.RemoteAddrString;
	if (!src || !*src) {
		Logger::Log(GetLogChannel(), "No remote address\n");
		LogCallEnd();
		return nullptr;
	}

	// 8-bit → 16-bit widen for the FString. The source is always "a.b.c.d:port"
	// (sprintf'd into RemoteAddrString in TickDispatch), so every byte is ASCII
	// and a plain zero-extending widen is correct — no MultiByteToWideChar +
	// std::wstring heap dance needed. RemoteAddrString is 32 chars; cap defensively.
	wchar_t wbuf[32];
	int n = 0;
	while (n < 31 && src[n] != '\0') {
		wbuf[n] = (wchar_t)(unsigned char)src[n];
		++n;
	}
	wbuf[n] = L'\0';

	FString__CreateFromWCharT::CallOriginal(Out, nullptr, wbuf);
	it->second.RemoteAddrFString = (FString*)Out;

	LogCallEnd();
	return Out;
}


// void LogFStringToFile(const char* filename, FString& str)
// {
//     if (str.Count > 1 && str.Data)
//     {
//         size_t len = wcstombs(NULL, str.Data, 0);
//         if (len != (size_t)-1)
//         {
//             char* buffer = (char*)malloc(len + 1);
//             wcstombs(buffer, str.Data, len + 1);
// 			LogToFile(filename, buffer);
//             free(buffer);
//         }
//         else
//         {
// 			LogToFile(filename, "FString conversion failed.");
//         }
//     }
// }




// std::wstring CommandLineParser::Utf8ToWide(const std::string &s) {
//     if (s.empty()) return {};
//     int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
//     std::wstring out(size, L'\0');
//     MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &out[0], size);
//     return out;
// }

