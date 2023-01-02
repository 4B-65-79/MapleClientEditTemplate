#include "Common.h"

// Exclude rarely-used stuff from Windows headers
// Important to define this before Windows.h is included in a project because of linker issues with the WinSock2 lib
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include "FakeModule.h"
#include "hooker.h"
#include "logger.h"
#include "winhooks.h"
#include "winhook_types.h"

Common* Common::_s_pInstance;
Common::Config* Common::_s_pConfig;

Common::Common(bool bHookWinLibs, std::function<void()> const& pPostMutexFunc, const char* sIP, const char* sOriginalIP)
{
	this->m_sRedirectIP = sIP;
	this->m_sOriginalIP = sOriginalIP;
	this->m_GameSock = INVALID_SOCKET;
	this->m_pProcTable = nullptr;
	this->m_bThemidaUnpacked = false;
	this->m_dwGetProcRetAddr = 0;
	this->m_pFakeHsModule = nullptr;

	if (!pPostMutexFunc)
	{
#if _DEBUG
		Log("Invalid function pointer passed to Common constructor.");
#endif
		return;
	}

	this->m_PostMutexFunc = pPostMutexFunc;

	if (Common::GetConfig()->InjectImmediately) // call post-unpack function right away
	{
		// call OnThemidaUnpack so the SleepAfterUnpackDuration will trigger
		// even if InjectImmediately is active
		this->OnThemidaUnpack();
	}

#if _DEBUG
	Log("Common created => Hook winsock libs: %s || IP: %s || Original IP: %s", (bHookWinLibs ? "Yes" : "No"), sIP, sOriginalIP);
#endif

	// required for proper injection
	INITWINHOOK("KERNEL32", "CreateMutexA", CreateMutexA_Original, CreateMutexA_t, WinHooks::CreateMutexA_Hook);


	if (this->GetConfig()->HookToggleInfo.ImGui_Enable) {
#if DIRECTX_VERSION == 8
		INITWINHOOK(
			"D3D8",
			"Direct3DCreate8",
			Direct3DCreateX_Original,
			Direct3DCreateX_t,
			WinHooks::DirectX::Direct3DCreateX_Hook
		);
#elif DIRECTX_VERSION == 9
		INITWINHOOK(
			"D3D9",
			"Direct3DCreate9",
			Direct3DCreateX_Original,
			Direct3DCreateX_t,
			WinHooks::DirectX::Direct3DCreateX_Hook
		);
#endif
	}

	if (Common::GetConfig()->MaplePatcherClass || Common::GetConfig()->MapleWindowClass || Common::GetConfig()->InjectImmediately)
	{
		INITWINHOOK("USER32", "CreateWindowExA", CreateWindowExA_Original, CreateWindowExA_t, WinHooks::CreateWindowExA_Hook);
	}

	if (Common::GetConfig()->LocaleSpoofValue)
	{
		INITWINHOOK("KERNEL32", "GetACP", GetACP_Original, GetACP_t, WinHooks::GetACP_Hook);
	}

	if (Common::GetConfig()->HookToggleInfo.OpenProcess_Logging)
	{
		INITWINHOOK("KERNEL32", "OpenProcess", OpenProcess_Original, OpenProcess_t, WinHooks::OpenProcess_Hook);
	}

	if (Common::GetConfig()->HookToggleInfo.CreateProcess_Logging)
	{
		INITWINHOOK("KERNEL32", "CreateProcessW", CreateProcessW_Original, CreateProcessW_t, WinHooks::CreateProcessW_Hook);
		INITWINHOOK("KERNEL32", "CreateProcessA", CreateProcessA_Original, CreateProcessA_t, WinHooks::CreateProcessA_Hook);
	}
	else if (Common::GetConfig()->MapleExitWindowWebUrl && *Common::GetConfig()->MapleExitWindowWebUrl)
	{
		INITWINHOOK("KERNEL32", "CreateProcessA", CreateProcessA_Original, CreateProcessA_t, WinHooks::CreateProcessA_Hook);
	}

	if (Common::GetConfig()->HookToggleInfo.OpenMutexA_Logging || Common::GetConfig()->HookToggleInfo.OpenMutexA_Spoof)
	{
		INITWINHOOK("KERNEL32", "OpenMutexA", OpenMutexA_Original, OpenMutexA_t, WinHooks::OpenMutexA_Hook);
	}

	if (Common::GetConfig()->HookToggleInfo.NtTerminateProc_Logging)
	{
		INITWINHOOK("NTDLL", "NtTerminateProcess", NtTerminateProcess_Original, NtTerminateProcess_t, WinHooks::NtTerminateProcess_Hook);
	}

	if (Common::GetConfig()->HookToggleInfo.RegCreateKeyA_Logging)
	{
		INITWINHOOK("KERNEL32", "RegCreateKeyExA", RegCreateKeyExA_Original, RegCreateKeyExA_t, WinHooks::RegCreateKeyExA_Hook);
	}

	if (Common::GetConfig()->HookToggleInfo.GetProcAddress_Logging)
	{
		INITWINHOOK("KERNEL32", "GetProcAddress", GetProcAddress_Original, GetProcAddress_t, WinHooks::GetProcAddress_Hook);
	}

	if (!bHookWinLibs) return;

	if (!sIP || !sOriginalIP)
	{
#if _DEBUG
		Log("Null IP string passed to Common constructor.");
#endif
		return;
	}

	INITWINHOOK("MSWSOCK", "WSPStartup", WSPStartup_Original, WSPStartup_t, WinHooks::WinSock::WSPStartup_Hook);
}

Common::~Common()
{
#if _DEBUG
	Log("Cleaning up common..");
#endif

	if (this->m_pFakeHsModule)
	{
		// TODO figure out some common library call to put this instead of in dll detach
		// CLogo constructor is pretty good but its not a library call so idk
		this->m_pFakeHsModule->DeleteModule();
	}

	if (this->m_GameSock != INVALID_SOCKET)
	{
#if _DEBUG
		Log("Closing socket..");
#endif

		this->m_pProcTable->lpWSPCloseSocket(this->m_GameSock, nullptr);
		this->m_GameSock = INVALID_SOCKET;
	}

	// TODO clean up ImGui stuff here
}

void Common::OnThemidaUnpack()
{
	if (this->m_bThemidaUnpacked) return;
	this->m_bThemidaUnpacked = true;

	if (Common::GetConfig()->SleepAfterUnpackDuration)
	{
		Log("Themida unpacked => sleeping for %d milliseconds.", Common::GetConfig()->SleepAfterUnpackDuration);
		Sleep(Common::GetConfig()->SleepAfterUnpackDuration);
	}

#if _DEBUG
	Log("Themida unpacked, editing memory..");
#endif

	this->m_PostMutexFunc();
}

void Common::SetImGuiDrawingFunc(std::function<void()> pDrawingFunc)
{
	WinHooks::DirectX::g_pDrawingFunc = pDrawingFunc;
}