#include "src/Utils/DebugWindow/DebugWindow.hpp"

// HWND DebugWindow::hCombo = nullptr;
// HWND DebugWindow::hOutput = nullptr;
HWND DebugWindow::hAutoRefreshCheckbox = nullptr;
HWND DebugWindow::hAddressInput = nullptr;
HWND DebugWindow::hSizeInput = nullptr;
HWND DebugWindow::hTypeSelect = nullptr;
HWND DebugWindow::hInstanceSelect = nullptr;
HWND DebugWindow::hApplyButton = nullptr;
HWND DebugWindow::hDumpBox = nullptr;
bool DebugWindow::bAutoRefresh = false;
std::string DebugWindow::SelectedInstance;
std::string DebugWindow::SelectedType;
uintptr_t DebugWindow::SelectedAddress;
int DebugWindow::SelectedSize = 100;
HINSTANCE DebugWindow::hInstance = nullptr;
// std::string DebugWindow::SelectedOption;

DWORD WINAPI DebugWindow::ModuleThread(LPVOID lpParam)
{
	hInstance = (HINSTANCE)lpParam;

	WNDCLASS wc{};
	wc.lpfnWndProc = DebugWindow::WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = TEXT("DebugWindowClass");
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		wc.lpszClassName,
		TEXT("Debug Window"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1920, 1000,
		nullptr, nullptr, hInstance, nullptr
	);

	ShowWindow(hwnd, SW_SHOW);

	hInstanceSelect = CreateWindowEx(
		0, TEXT("COMBOBOX"), nullptr,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
		20, 20, 200, 200,
		hwnd, (HMENU)InstanceSelectId, hInstance, nullptr
	);

	hTypeSelect = CreateWindowEx(
		0, TEXT("COMBOBOX"), nullptr,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
		300, 20, 200, 200,
		hwnd, (HMENU)TypeSelectId, hInstance, nullptr
	);

	for (auto& [label, ptr] : Types)
	{
		SendMessageA(hTypeSelect, CB_ADDSTRING, 0, (LPARAM)label.c_str());
	}


	hAddressInput = CreateWindowEx(
		0,
		TEXT("EDIT"),
		TEXT("0x00000000"),
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		550, 20, 200, 25,
		hwnd, (HMENU)AddressInputId, hInstance, nullptr
	);

	hSizeInput = CreateWindowEx(
		0,
		TEXT("EDIT"),
		TEXT("100"),
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		800, 20, 50, 25,
		hwnd, (HMENU)SizeInputId, hInstance, nullptr
	);

	hApplyButton = CreateWindowEx(
		0,
		TEXT("BUTTON"),
		TEXT("Apply"),
		WS_CHILD | WS_VISIBLE,
		870, 20, 100, 25,
		hwnd,
		(HMENU)ApplyButtonId, // button ID
		hInstance,
		nullptr
	);

	DebugWindow::hAutoRefreshCheckbox = CreateWindowEx(
		0,
		TEXT("BUTTON"),
		TEXT("Auto Refresh"),
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		1000, 20, 150, 25,      // position + size
		hwnd,
		(HMENU)AutoRefreshCheckboxId,
		hInstance,
		nullptr
	);

	DebugWindow::hDumpBox = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		TEXT("EDIT"),
		TEXT(""),
		WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_READONLY | WS_VSCROLL,
		20, 50, 1880, 960,        // position + size
		hwnd,
		nullptr,
		hInstance,
		nullptr
	);

	SetTimer(hwnd, 1, 100, nullptr);

	// Message loop
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

void DebugWindow::RefreshOptions()
{
	if (!hInstanceSelect)
		return;

	// Clear all items
	SendMessage(hInstanceSelect, CB_RESETCONTENT, 0, 0);

	// Add new items
	for (auto& [label, ptr] : Instances)
	{
		SendMessageA(hInstanceSelect, CB_ADDSTRING, 0, (LPARAM)label.c_str());
	}
}

LRESULT CALLBACK DebugWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
			{
				if ((HWND)lParam == hInstanceSelect && HIWORD(wParam) == CBN_SELCHANGE)
				{
					int idx = SendMessage(hInstanceSelect, CB_GETCURSEL, 0, 0);

					char buffer[256];
					SendMessageA(hInstanceSelect, CB_GETLBTEXT, idx, (LPARAM)buffer);
					SelectedInstance = buffer;

					SelectedType = Instances[SelectedInstance].type;
					SelectedAddress = Instances[SelectedInstance].address;

					int idxType = SendMessageA(hTypeSelect, CB_FINDSTRINGEXACT, -1, (LPARAM)SelectedType.c_str());
					if (idxType != CB_ERR) {
						SendMessage(hTypeSelect, CB_SETCURSEL, idxType, 0);
					}

					char addrText[32];
					sprintf(addrText, "0x%llX", (unsigned long long)SelectedAddress);
					SetWindowTextA(hAddressInput, addrText);

					// change hTypeSelect's selected option
					// change hAddressInput's value

					ShowDebugInfo();
				}

				if ((HWND)lParam == hTypeSelect && HIWORD(wParam) == CBN_SELCHANGE)
				{
					int idx = SendMessage(hTypeSelect, CB_GETCURSEL, 0, 0);

					char buffer[256];
					SendMessageA(hTypeSelect, CB_GETLBTEXT, idx, (LPARAM)buffer);
					SelectedType = buffer;

					ShowDebugInfo();

					// char out[256];
					// sprintf(out, "Selected: %s → %p", SelectedOption.c_str(), (void*)value);
					// SetWindowTextA(hOutput, out);

				}

				if ((HWND)lParam == hApplyButton && HIWORD(wParam) == BN_CLICKED)
				{
					char buf[64];
					GetWindowTextA(DebugWindow::hAddressInput, buf, 64);

					unsigned long long value = 0;
					sscanf(buf, "%llx", &value);

					DebugWindow::SelectedAddress = (uintptr_t)value;

					char buf2[64];
					GetWindowTextA(DebugWindow::hSizeInput, buf2, 64);
					DebugWindow::SelectedSize = atoi(buf2);

					ShowDebugInfo();

					return 0;
				}

				if (LOWORD(wParam) == AutoRefreshCheckboxId && HIWORD(wParam) == BN_CLICKED)
				{
					// Read state
					LRESULT state = SendMessage(DebugWindow::hAutoRefreshCheckbox, BM_GETCHECK, 0, 0);

					DebugWindow::bAutoRefresh = (state == BST_CHECKED);
				}
				break;


			}
		case WM_SIZE:
			{
				int width  = LOWORD(lParam);
				int height = HIWORD(lParam);

				if (DebugWindow::hDumpBox)
				{
					MoveWindow(
						DebugWindow::hDumpBox,
						20,               // X
						50,              // Y (below ComboBox)
						width - 40,       // width with margin
						height - 50,     // height with bottom margin
						TRUE
					);
				}

				return 0;
			}

		case WM_TIMER:
			{
				if (DebugWindow::bAutoRefresh) {
					ShowDebugInfo();
				}
				return 0;
			}

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DebugWindow::ShowDebugInfo() {

	if (SelectedType == "" || SelectedAddress == 0) {
		return;
	}

	uintptr_t value = DebugWindow::SelectedAddress;
	std::string type = DebugWindow::SelectedType;

	std::string dump;

	// if (SelectedOption == "RandomSMSettings") {
	// 	char buffer[1024];
	//
	// 	void* cmarshal = (void*)value;
	// 	void* cmarshalentry = *(void**)((char*)cmarshal + 0x10);
	// }
	if (type == "Raw") {
		char buffer[131072];
		char* base = (char*)SelectedAddress;
		for (int i = 0; i < SelectedSize; i += 4) {
			sprintf(buffer, "0x%04X:   %08X\n", i, *(int*)(base + i));
			dump += buffer;
		}
	}
	if (type == "RandomSMManager") {
		ATgRandomSMManager* RandomSMManager = reinterpret_cast<ATgRandomSMManager*>(value);
		char buffer[1024];
		for (int i = 0; i < RandomSMManager->m_RandomSMActorInfo.Num(); i++) {
			sprintf(buffer, "RandomSMActorInfo[%d]: {nType: %d, nCount: %d}\n", i, RandomSMManager->m_RandomSMActorInfo.Data[i].nType, RandomSMManager->m_RandomSMActorInfo.Data[i].nCount);
			dump += buffer;
		}
	}
	if (type == "Game") {
		ATgGame* game = reinterpret_cast<ATgGame*>(value);
		char buffer[1024];
		sprintf(buffer, "m_GameType: %d\n \
s_bGameInitialized: %d\n \
m_eTimerState: %d\n \
m_fMissionTime: %f\n \
m_fGameMissionTime: %f\n \
s_fMissionTimerStartedAt: %f\n \
s_MissionTimeAccumulator: %f\n \
",
		  game->m_GameType,
		  game->s_bGameInitialized,
		  game->m_eTimerState,
		  game->m_fMissionTime,
		  game->m_fGameMissionTime,
		  game->s_fMissionTimerStartedAt,
		  game->s_MissionTimeAccumulator
		  );


		dump += buffer;
	}
	if (type == "GameReplicationInfo") {
		ATgRepInfo_Game* gamerep = reinterpret_cast<ATgRepInfo_Game*>(value);
		char buffer[1024];
		sprintf(buffer, "r_GameType: %d\n \
r_nMissionTimerState: %d\n \
r_bIsRaid: %d\n \
r_bIsMission: %d\n \
r_bIsPVP: %d\n \
r_bIsTraining: %d\n \
r_bIsTutorialMap: %d\n \
r_bIsArena: %d\n \
r_bIsMatch: %d\n \
r_bIsTerritoryMap: %d\n \
r_bIsOpenWorld: %d\n \
r_bAllowBuildMorale: %d\n \
r_bActiveCombat: %d\n \
r_bAllowPlayerRelease: %d\n \
r_bDefenseAlarm: %d\n \
r_bInOverTime: %d\n \
s_fActiveCombatTime: %f\n \
r_nSecsToAutoReleaseAttackers: %d\n \
r_nSecsToAutoReleaseDefenders: %d\n \
r_nReleaseDelay: %d\n \
r_fGameSpeedModifier: %f\n \
r_nPointsToWin: %d\n \
r_nVictoryBonusLives: %d\n \
r_nRaidDefenderRespawnBonus: %d\n \
r_nRaidAttackerRespawnBonus: %d\n \
r_fMissionRemainingTime: %f\n \
r_nMissionTimerStateChange: %d\n \
c_fMissionTime: %f\n \
c_fMissionTimeSeconds: %f\n \
r_nRoundNumber: %d\n \
r_nMaxRoundNumber: %d\n \
r_fServerTimeLastUpdate: %f\n \
",
		  gamerep->r_GameType,
		  gamerep->r_nMissionTimerState,
		  gamerep->r_bIsRaid,
		  gamerep->r_bIsMission,
		  gamerep->r_bIsPVP,
		  gamerep->r_bIsTraining,
		  gamerep->r_bIsTutorialMap,
		  gamerep->r_bIsArena,
		  gamerep->r_bIsMatch,
		  gamerep->r_bIsTerritoryMap,
		  gamerep->r_bIsOpenWorld,
		  gamerep->r_bAllowBuildMorale,
		  gamerep->r_bActiveCombat,
		  gamerep->r_bAllowPlayerRelease,
		  gamerep->r_bDefenseAlarm,
		  gamerep->r_bInOverTime,
		  gamerep->s_fActiveCombatTime,
		  gamerep->r_nSecsToAutoReleaseAttackers,
		  gamerep->r_nSecsToAutoReleaseDefenders,
		  gamerep->r_nReleaseDelay,
		  gamerep->r_fGameSpeedModifier,
		  gamerep->r_nPointsToWin,
		  gamerep->r_nVictoryBonusLives,
		  gamerep->r_nRaidDefenderRespawnBonus,
		  gamerep->r_nRaidAttackerRespawnBonus,
		  gamerep->r_fMissionRemainingTime,
		  gamerep->r_nMissionTimerStateChange,
		  gamerep->c_fMissionTime,
		  gamerep->c_fMissionTimeSeconds,
		  gamerep->r_nRoundNumber,
		  gamerep->r_nMaxRoundNumber,
		  gamerep->r_fServerTimeLastUpdate
		  );


		dump += buffer;
	}

	SetWindowTextA(DebugWindow::hDumpBox, dump.c_str());
}

