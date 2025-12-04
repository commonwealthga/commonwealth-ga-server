#include "src/pch.hpp"
#include "src/GameServer/IpDrv/NetConnection/LowLevelGetRemoteAddress/NetConnection__LowLevelGetRemoteAddress.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Core/FString/CreateFromWCharT/FString__CreateFromWCharT.hpp"
#include "src/Utils/Logger/Logger.hpp"

void* __fastcall NetConnection__LowLevelGetRemoteAddress::Call(UNetConnection* NetConnection, void* edx, void* Out) {
	LogCallBegin();

    uint32_t *pParam = (uint32_t*)Out;

    if (pParam) {
        pParam[0] = 0;
        pParam[1] = 0;
        pParam[2] = 0;
    }

    int32_t index = (int32_t)NetConnection;
    auto it = GClientConnectionsData.find(index);
    if (it == GClientConnectionsData.end()) {
		Logger::Log(GetLogChannel(), "Connection not found\n");
		LogCallEnd();
        return nullptr;
    }

	if (it->second.bClosed) {
		Logger::Log(GetLogChannel(), "Connection closed\n");
	// } else if (it->second.RemoteAddrFString != nullptr) {
	// 	Logger::Log(GetLogChannel(), "Remote address already set: 0x%p ", it->second.RemoteAddrFString);
	//
	// 	if (it->second.RemoteAddrFString->Count > 1 && it->second.RemoteAddrFString->Data) {
	// 		size_t len = wcstombs(NULL, it->second.RemoteAddrFString->Data, 0);
	// 		if (len != (size_t)-1) {
	// 			char* buffer = (char*)malloc(len + 1);
	// 			wcstombs(buffer, it->second.RemoteAddrFString->Data, len + 1);
	// 			Logger::Log(GetLogChannel(), "%s\n", buffer);
	// 			free(buffer);
	// 		} else {
	// 			// LogToFile(filename, "FString conversion failed.");
	// 		}
	// 	}
	//
	// 	Out = it->second.RemoteAddrFString;
	// 	LogCallEnd();
	// 	return;
	}

    wchar_t local_50[32];
    local_50[0] = L'\0';

    const char* src = it->second.RemoteAddrString;
    if (!src) {
		// FString__CreateFromWCharT::CallOriginal(Out, nullptr, local_50);
		Logger::Log(GetLogChannel(), "No remote address\n");
		LogCallEnd();
        return nullptr;
    }

	const std::string s = src;
    int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &out[0], size);

	// for (int i = 0; i < out.size(); ++i) {
	// 	local_50[i] = out[i];
	// }
	//    local_50[size] = L'\0';


    // int maxChars = 31;
    // int i = 0;
    // for (; i < maxChars && src[i] != '\0'; ++i) {
    //     local_50[i] = (wchar_t)(unsigned char)src[i];
    // }
    // local_50[i] = L'\0';

	FString__CreateFromWCharT::CallOriginal(Out, nullptr, (void*)out.c_str());
	// FString__CreateFromWCharT::CallOriginal(Out, nullptr, local_50);

	it->second.RemoteAddrFString = (FString*)Out;


	if (it->second.RemoteAddrFString->Count > 1 && it->second.RemoteAddrFString->Data) {
		size_t len = wcstombs(NULL, it->second.RemoteAddrFString->Data, 0);
		if (len != (size_t)-1) {
			char* buffer = (char*)malloc(len + 1);
			wcstombs(buffer, it->second.RemoteAddrFString->Data, len + 1);
			Logger::Log(GetLogChannel(), "Built remote address FString [0x%p]: %s\n", it->second.RemoteAddrFString, buffer);
			free(buffer);
		} else {
			Logger::Log(GetLogChannel(), "Built remote address FString [0x%p]\n", it->second.RemoteAddrFString);
			// LogToFile(filename, "FString conversion failed.");
		}
	}

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

