#pragma once

#include "src/pch.hpp"

struct DebugInstance {
	uintptr_t address;
	std::string type;
};

class DebugWindow {
public:
    // static inline std::map<std::string, uintptr_t> Options{}; // <label, pointer>
	static inline std::map<std::string, DebugInstance> Instances{};
	static inline std::map<std::string, std::string> Types{
		{"Raw", "Raw"},
		{"RandomSMManager", "RandomSMManager"},
		{"Game", "Game"},
		{"GameReplicationInfo", "GameReplicationInfo"},
	};

	static inline int AutoRefreshCheckboxId = 1001;
	static inline int AddressInputId = 1002;
	static inline int SizeInputId = 1003;
	static inline int TypeSelectId = 1004;
	static inline int InstanceSelectId = 1005;
	static inline int ApplyButtonId = 1006;

    // static HWND hCombo;
    // static HWND hOutput;
    static HINSTANCE hInstance;

	static HWND hAutoRefreshCheckbox;
	static HWND hAddressInput;
	static HWND hSizeInput;
	static HWND hTypeSelect;
	static HWND hInstanceSelect;
	static HWND hApplyButton;

	static HWND hDumpBox;

	static bool bAutoRefresh;
	static std::string SelectedInstance;
	static std::string SelectedType;
	static uintptr_t SelectedAddress;
	static int SelectedSize;

	// static std::string SelectedOption;

    static void RefreshOptions(); // repopulate combobox

    static DWORD WINAPI ModuleThread(LPVOID lpParam);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static void ShowDebugInfo();
};

