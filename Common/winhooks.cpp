#include <intrin.h>
#include "winhooks.h"
#include "winhook_types.h"
#include "hooker.h"
#include "logger.h"
#include "memedit.h"
#include "FakeModule.h"
#include "Common.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"

#if DIRECTX_VERSION

#if DIRECTX_VERSION == 8
#pragma comment(lib, "d3d8")
#include "imgui/imgui_impl_dx8.h"
#elif DIRECTX_VERSION == 9
#pragma comment(lib, "d3d9")
#include "imgui/imgui_impl_dx9.h"
#endif

// forward declaration of imgui wndproc
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#if DIRECTX_VERSION == 8:
#define IMGUI_DX_NEW_FRAME() ImGui_ImplDX8_NewFrame()
#define IMGUI_DX_RENDER_DRAW_DATA(drawData) ImGui_ImplDX8_RenderDrawData(drawData)
#define IMGUI_DX_INIT(device) ImGui_ImplDX8_Init(device)

#define DX_CREATEDEVICE_INTERFACE_OFFSET 15
#define DX_INTERFACE_OFFSET__ENDSCENE 34
#elif DIRECTX_VERSION == 9
#define IMGUI_DX_NEW_FRAME() ImGui_ImplDX9_NewFrame()
#define IMGUI_DX_RENDER_DRAW_DATA(drawData) ImGui_ImplDX9_RenderDrawData(drawData)
#define IMGUI_DX_INIT(device) ImGui_ImplDX9_Init(device)

#define DX_INTERFACE_OFFSET__CREATEDEVICE 16
#define DX_INTERFACE_OFFSET__ENDSCENE 42
#endif

#endif

namespace WinHooks
{
	// fix returnaddress func
	// https://docs.microsoft.com/en-us/cpp/intrinsics/returnaddress?view=msvc-160
#pragma intrinsic(_ReturnAddress)

// link socket library
#pragma comment(lib, "WS2_32.lib")

// deprecated api call warning
#pragma warning(disable : 4996)

#if DIRECTX_VERSION

	namespace DirectX
	{
		WNDPROC g_pWndProc;
		std::function<void()> g_pDrawingFunc;
		HWND					g_hWnd;
		LPDIRECT3DX             g_pD3D;
		IDirect3DDeviceX* g_pd3dDevice;
		D3DPRESENT_PARAMETERS   g_d3dpp;

		/// <summary>
		/// This isn't a D3D function but since it's related to D3D (in this project) I'm calling it that.
		/// This will only get hooked and used if ImGui_Enable is enabled in the common config.
		/// This exists to pass window commands to ImGui before it gets passed into MapleStory.
		/// If we don't do this, the ImGui windows will not be able to accept any input.
		/// </summary>
		LRESULT D3D_WndProc_Hook(
			HWND hWnd,
			UINT msg,
			WPARAM wParam,
			LPARAM lParam
		) {
			// pass control to imgui first to see if it wants to continue
			if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
				return true;

			// if this is a mouse message and imgui wants control, 
			// we do not pass control back to maple afterwards
			switch (msg) {
				//case WM_MOUSELEAVE:
				//case WM_MOUSEMOVE:
				//case WM_SETCURSOR:
			case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
			case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_XBUTTONUP:
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_SETFOCUS:
			case WM_KILLFOCUS:
			case WM_CHAR:
				if (ImGui::GetCurrentContext())
				{
					ImGuiIO& io = ImGui::GetIO();
					if (io.WantCaptureMouse || io.WantCaptureKeyboard)
						return true;
				}

				break;
			}

			// send control to the original maplestory wndproc function
			return g_pWndProc(hWnd, msg, wParam, lParam);
		}

		HRESULT WINAPI D3DX__CreateDevice_Hook(
			LPDIRECT3DX pThis,
			UINT Adapter,
			D3DDEVTYPE DeviceType,
			HWND hFocusWindow,
			DWORD BehaviorFlags,
			D3DPRESENT_PARAMETERS* pPresentationParameters,
			IDirect3DDeviceX** ppReturnedDeviceInterface
		)
		{
#if _DEBUG
			Log("CreateDevice detour triggered by 0x%0X.", _ReturnAddress());
#endif

			auto hrRet = D3DX__CreateDevice_Original(pThis, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

			// get instance handle to maplestory
			HINSTANCE hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hFocusWindow, GWLP_HINSTANCE));

			// get window class structure -- we need the WndProc pointer so we can replace it with our own
			WNDCLASSEXA wc;
			auto bClassInfoSuccess = GetClassInfoEx(hInst, Common::GetConfig()->MapleWindowClass, &wc);

#if _DEBUG
			if (!bClassInfoSuccess)
			{
				Log("[D3D8__CreateDevice_Hook]: Failed GetClassInfoEx. Error: %d", GetLastError());
			}

			Log("[D3D8__CreateDevice_Hook]: hWnd: 0x%08X. lpfnWndProc: 0x%08X.", hFocusWindow, wc.lpfnWndProc);
#endif

			// save the original wndproc so we can call it from our detour
			g_pWndProc = wc.lpfnWndProc;

			// detour the wndproc with our wndproc hook
			SetWindowLongPtr(hFocusWindow, GWLP_WNDPROC, reinterpret_cast<LONG>(D3D_WndProc_Hook));

			g_hWnd = hFocusWindow;
			D3DX__EndScene_Original = (D3DX__EndScene_t)HookVTableFunction(
				*ppReturnedDeviceInterface,
				D3DX__EndScene_Hook,
				DX_INTERFACE_OFFSET__ENDSCENE // magic number indicates offset of IDirect3D9::CreateDevice
			);
			return hrRet;
		}

		IDirect3DX* WINAPI Direct3DCreateX_Hook(UINT SDKVersion)
		{
#if _DEBUG
			Log("D3D hooked");
#endif

			IDirect3DX* pD3D = Direct3DCreateX_Original(SDKVersion);

			g_pD3D = pD3D;
			D3DX__CreateDevice_Original = (D3DX__CreateDevice_t)HookVTableFunction(
				g_pD3D,
				D3DX__CreateDevice_Hook,
				DX_INTERFACE_OFFSET__CREATEDEVICE // magic number indicates offset of IDirect3D9::CreateDevice
			);
			return pD3D;
		}

		HRESULT WINAPI D3DX__EndScene_Hook(LPDIRECT3DDEVICEX pThis)
		{
			static BOOL s_bInitialized = false;

			if (!g_pd3dDevice)
				g_pd3dDevice = pThis;

			if (!s_bInitialized)
			{
#if _DEBUG
				Log("Initializing ImGUI..");
#endif
				ImGui::CreateContext();
				ImGuiIO& io = ImGui::GetIO();
				io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

				ImGui_ImplWin32_Init(g_hWnd);
				IMGUI_DX_INIT(g_pd3dDevice);
				s_bInitialized = true;
			}
			else
			{
				IMGUI_DX_NEW_FRAME();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				if (g_pDrawingFunc)
				{
					g_pDrawingFunc(); 
				}

				ImGui::EndFrame();
				ImGui::Render();
				IMGUI_DX_RENDER_DRAW_DATA(ImGui::GetDrawData());
			}

			return D3DX__EndScene_Original(pThis);
		}
	}

#endif 

	/// <summary>
	/// Used to map out imports used by MapleStory.
	/// The log output can be used to reconstruct the _ZAPIProcAddress struct
	/// ZAPI struct is the dword before the while loop when searching for aob: 68 FE 00 00 00 ?? 8D
	/// </summary>
	FARPROC WINAPI GetProcAddress_Hook(HMODULE hModule, LPCSTR lpProcName)
	{
		if (Common::GetInstance()->m_bThemidaUnpacked)
		{
			DWORD dwRetAddr = reinterpret_cast<DWORD>(_ReturnAddress());

			if (Common::GetInstance()->m_dwGetProcRetAddr != dwRetAddr)
			{
				Common::GetInstance()->m_dwGetProcRetAddr = dwRetAddr;

				Log("[GetProcAddress] Detected library loading from %08X.", dwRetAddr);
			}

			Log("[GetProcAddress] => %s", lpProcName);
		}

		return GetProcAddress_Original(hModule, lpProcName);
	}

	/// <summary>
	/// CreateMutexA is the first Windows library call after the executable unpacks itself.
	/// We hook this function to do all our memory edits and hooks when it's called.
	/// </summary>
	HANDLE WINAPI CreateMutexA_Hook(
		LPSECURITY_ATTRIBUTES lpMutexAttributes,
		BOOL				  bInitialOwner,
		LPCSTR				  lpName
	)
	{
#if _DEBUG
		Log("CreateMutexA Triggered: %s", lpName);
#endif
		if (!CreateMutexA_Original)
		{
			Log("Original CreateMutex pointer corrupted. Failed to return mutex value to calling function.");

			return nullptr;
		}
		else if (lpName && strstr(lpName, Common::GetConfig()->MapleMutex))
		{
			if (Common::GetConfig()->AllowMulticlient)
			{
				// from https://github.com/pokiuwu/AuthHook-v203.4/blob/AuthHook-v203.4/Client176/WinHook.cpp

				char szMutex[128];
				int nPID = GetCurrentProcessId();

				sprintf_s(szMutex, "%s-%d", lpName, nPID);
				lpName = szMutex;
			}

			if (!Common::GetConfig()->InjectImmediately)
			{
				Common::GetInstance()->OnThemidaUnpack();
			}

			return CreateMutexA_Original(lpMutexAttributes, bInitialOwner, lpName);
		}

		return CreateMutexA_Original(lpMutexAttributes, bInitialOwner, lpName);
	}

	/// <summary>
	/// In some versions, Maple calls this library function to check if the anticheat has started.
	/// We can spoof this and return a fake handle for it to close.
	/// </summary>
	HANDLE WINAPI OpenMutexA_Hook(
		DWORD  dwDesiredAccess,
		BOOL   bInitialOwner,
		LPCSTR lpName
	)
	{
#if _DEBUG
		Log("Opening mutex %s", lpName);
#endif

		if (strstr(lpName, "meteora")) // make sure we only override hackshield
		{
			Log("Detected HS mutex => spoofing.");

			Common::GetInstance()->m_pFakeHsModule = new FakeModule();

			if (!Common::GetInstance()->m_pFakeHsModule->CreateModule("ehsvc.dll"))
			{
				Log("Unable to create fake HS module.");
			}
			else
			{
				Log("Fake HS module loaded.");
			}

			// return handle to a spoofed mutex so it can close the handle
			return CreateMutexA_Original(NULL, TRUE, "FakeMutex1");
		}
		else // TODO add second mutex handling
		{
			return OpenMutexA_Original(dwDesiredAccess, bInitialOwner, lpName);
		}
	}

	/// <summary>
	/// Used to track what maple is trying to start (mainly for anticheat modules).
	/// </summary>
	BOOL WINAPI CreateProcessW_Hook(
		LPCWSTR               lpApplicationName,
		LPWSTR                lpCommandLine,
		LPSECURITY_ATTRIBUTES lpProcessAttributes,
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
		BOOL                  bInheritHandles,
		DWORD                 dwCreationFlags,
		LPVOID                lpEnvironment,
		LPCWSTR               lpCurrentDirectory,
		LPSTARTUPINFOW        lpStartupInfo,
		LPPROCESS_INFORMATION lpProcessInformation
	)
	{
		if (Common::GetConfig()->HookToggleInfo.CreateProcess_Logging)
		{
			auto sAppName = lpApplicationName ? lpApplicationName : L"Null App Name";
			auto sArgs = lpCommandLine ? lpCommandLine : L"Null Args";

			Log("CreateProcessW -> %s : %s", sAppName, sArgs);
		}

		return CreateProcessW_Original(
			lpApplicationName, lpCommandLine, lpProcessAttributes,
			lpThreadAttributes, bInheritHandles, dwCreationFlags,
			lpEnvironment, lpCurrentDirectory, lpStartupInfo,
			lpProcessInformation
		);
	}

	/// <summary>
	/// Used same as above and also to kill/redirect some web requests.
	/// </summary>
	BOOL WINAPI CreateProcessA_Hook(
		LPCSTR                lpApplicationName,
		LPSTR                 lpCommandLine,
		LPSECURITY_ATTRIBUTES lpProcessAttributes,
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
		BOOL                  bInheritHandles,
		DWORD                 dwCreationFlags,
		LPVOID                lpEnvironment,
		LPCSTR                lpCurrentDirectory,
		LPSTARTUPINFOA        lpStartupInfo,
		LPPROCESS_INFORMATION lpProcessInformation)
	{
#if MAPLETRACKING_CREATE_PROCESS
		auto sAppName = lpApplicationName ? lpApplicationName : "Null App Name";
		auto sArgs = lpCommandLine ? lpCommandLine : "Null Args";

		Log("CreateProcessA -> %s : %s", sAppName, sArgs);
#endif

		if (Common::GetConfig()->MapleExitWindowWebUrl && strstr(lpCommandLine, Common::GetConfig()->MapleExitWindowWebUrl))
		{
			Log("[CreateProcessA] [%08X] Killing web request to: %s", _ReturnAddress(), lpApplicationName);
			return FALSE; // ret value doesn't get used by maple after creating web requests as far as i can tell
	}

		return CreateProcessA_Original(
			lpApplicationName, lpCommandLine, lpProcessAttributes,
			lpThreadAttributes, bInheritHandles, dwCreationFlags,
			lpEnvironment, lpCurrentDirectory, lpStartupInfo,
			lpProcessInformation
		);
}

	/// <summary>
	/// Same as CreateProcessW
	/// </summary>
	HANDLE WINAPI CreateThread_Hook(
		LPSECURITY_ATTRIBUTES   lpThreadAttributes,
		SIZE_T                  dwStackSize,
		LPTHREAD_START_ROUTINE  lpStartAddress,
		__drv_aliasesMem LPVOID lpParameter,
		DWORD                   dwCreationFlags,
		LPDWORD                 lpThreadId
	)
	{
		return CreateThread_Original(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
	}

	/// <summary>
	/// Used to track what processes Maple opens.
	/// </summary>
	HANDLE WINAPI OpenProcess_Hook(
		DWORD dwDesiredAccess,
		BOOL  bInheritHandle,
		DWORD dwProcessId
	)
	{
		Log("OpenProcess -> PID: %d - CallAddy: %08X", dwProcessId, _ReturnAddress());

		return OpenProcess_Original(dwDesiredAccess, bInheritHandle, dwProcessId);
	}

	/// <summary>
	/// This library call is used by nexon to determine the locale of the connecting clients PC. We spoof it.
	/// </summary>
	/// <returns></returns>
	UINT WINAPI GetACP_Hook() // AOB: FF 15 ?? ?? ?? ?? 3D ?? ?? ?? 00 00 74 <- library call inside winmain func
	{
		UINT uiNewLocale = Common::GetConfig()->LocaleSpoofValue;

		if (!uiNewLocale) return GetACP_Original(); // should not happen cuz we dont hook if value is zero

		// we dont wanna unhook until after themida is unpacked
		// because if themida isn't unpacked then the call we are intercepting is not from maple
		if (Common::GetInstance()->m_bThemidaUnpacked)
		{
			DWORD dwRetAddr = reinterpret_cast<DWORD>(_ReturnAddress());

			// return address should be a cmp eax instruction because ret value is stored in eax
			// and nothing else should happen before the cmp
			if (*MemEdit::ReadValue<BYTE>(dwRetAddr) == x86CMPEAX)
			{
				uiNewLocale = *MemEdit::ReadValue<DWORD>(dwRetAddr + 1); // check value is always 4 bytes

				Log("[GetACP] Found desired locale: %d", uiNewLocale);
			}
			else
			{
				Log("[GetACP] Unable to automatically determine locale, using stored locale: %d", uiNewLocale);
			}

			Log("[GetACP] Locale spoofed to %d, unhooking. Calling address: %08X", uiNewLocale, dwRetAddr);

			if (!SetHook(FALSE, reinterpret_cast<void**>(&GetACP_Original), GetACP_Hook))
			{
				Log("Failed to unhook GetACP.");
			}
		}

		return uiNewLocale;
	}

	/// <summary>
	/// Blocks the startup patcher "Play!" window and forces the login screen to be minimized
	/// </summary>
	HWND WINAPI CreateWindowExA_Hook(
		DWORD     dwExStyle,
		LPCSTR    lpClassName,
		LPCSTR    lpWindowName,
		DWORD     dwStyle,
		int       X,
		int       Y,
		int       nWidth,
		int       nHeight,
		HWND      hWndParent,
		HMENU     hMenu,
		HINSTANCE hInstance,
		LPVOID    lpParam
	)
	{
		Log("[CreateWindowExA] => %s - %s", lpClassName, lpWindowName);

		if (Common::GetConfig()->MaplePatcherClass && strstr(lpClassName, Common::GetConfig()->MaplePatcherClass))
		{
			Log("Bypassing patcher window..");

			if (Common::GetConfig()->InjectImmediately)
			{
				Common::GetInstance()->OnThemidaUnpack();
			}

			return NULL;
		}
		else
		{
			if (Common::GetConfig()->ForceWindowedOnStart
				&& Common::GetConfig()->MapleWindowClass
				&& strstr(lpClassName, Common::GetConfig()->MapleWindowClass))
			{
				dwExStyle = 0;
				dwStyle = 0xCA0000;
			}

			if (Common::GetConfig()->InjectImmediately)
			{
				Common::GetInstance()->OnThemidaUnpack();
			}

			return CreateWindowExA_Original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		}
	}

	/// <summary>
	/// We use this function to track what memory addresses are killing the process.
	/// There are more ways that Maple kills itself, but this is one of them.
	/// </summary>
	LONG NTAPI NtTerminateProcess_Hook(
		HANDLE hProcHandle,
		LONG   ntExitStatus
	)
	{
		Log("NtTerminateProcess: %08X", unsigned(_ReturnAddress()));

		return NtTerminateProcess_Original(hProcHandle, ntExitStatus);
	}

	/// <summary>
	/// Maplestory saves registry information (config stuff) for a number of things. This can be used to track that.
	/// </summary>
	LSTATUS WINAPI RegCreateKeyExA_Hook(
		HKEY                        hKey,
		LPCSTR                      lpSubKey,
		DWORD                       Reserved,
		LPSTR                       lpClass,
		DWORD                       dwOptions,
		REGSAM                      samDesired,
		const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		PHKEY                       phkResult,
		LPDWORD                     lpdwDisposition
	)
	{
		Log("RegCreateKeyExA - Return address: %d", _ReturnAddress());

		return RegCreateKeyExA_Original(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
	}

	namespace WinSock
	{

		/// <summary>
		/// 
		/// </summary>
		INT WSPAPI WSPConnect_Hook(
			SOCKET				   s,
			const struct sockaddr* name,
			int					   namelen,
			LPWSABUF			   lpCallerData,
			LPWSABUF			   lpCalleeData,
			LPQOS				   lpSQOS,
			LPQOS				   lpGQOS,
			LPINT				   lpErrno
		)
		{
			char szAddr[50];
			DWORD dwLen = 50;
			WSAAddressToString((sockaddr*)name, namelen, NULL, szAddr, &dwLen);

			sockaddr_in* service = (sockaddr_in*)name;

#if MAPLETRACKING_WSPCONN_PRINT
			Log("WSPConnect IP Detected: %s", szAddr);
#endif

			if (strstr(szAddr, Common::GetInstance()->m_sOriginalIP))
			{
				Log("Detected and rerouting socket connection to IP: %s", Common::GetInstance()->m_sRedirectIP);
				service->sin_addr.S_un.S_addr = inet_addr(Common::GetInstance()->m_sRedirectIP);
				Common::GetInstance()->m_GameSock = s;
		}

			return Common::GetInstance()->m_pProcTable->lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
	}

		/// <summary>
		/// 
		/// </summary>
		INT WSPAPI WSPGetPeerName_Hook(
			SOCKET			 s,
			struct sockaddr* name,
			LPINT			 namelen,
			LPINT			 lpErrno
		)
		{
			int nRet = Common::GetInstance()->m_pProcTable->lpWSPGetPeerName(s, name, namelen, lpErrno);

			if (nRet != SOCKET_ERROR)
			{
				char szAddr[50];
				DWORD dwLen = 50;
				WSAAddressToString((sockaddr*)name, *namelen, NULL, szAddr, &dwLen);

				sockaddr_in* service = (sockaddr_in*)name;

				USHORT nPort = ntohs(service->sin_port);

				if (s == Common::GetInstance()->m_GameSock)
				{
					char szAddr[50];
					DWORD dwLen = 50;
					WSAAddressToString((sockaddr*)name, *namelen, NULL, szAddr, &dwLen);

					sockaddr_in* service = (sockaddr_in*)name;

					u_short nPort = ntohs(service->sin_port);

					service->sin_addr.S_un.S_addr = inet_addr(Common::GetInstance()->m_sRedirectIP);

					Log("WSPGetPeerName => IP Replaced: %s -> %s", szAddr, Common::GetInstance()->m_sOriginalIP);
				}
				else
				{
					Log("WSPGetPeerName => IP Ignored: %s:%d", szAddr, nPort);
				}
			}
			else
			{
				Log("WSPGetPeerName Socket Error: %d", nRet);
			}

			return nRet;
		}

		/// <summary>
		/// 
		/// </summary>
		INT WSPAPI WSPCloseSocket_Hook(
			SOCKET s,
			LPINT  lpErrno
		)
		{
			int nRet = Common::GetInstance()->m_pProcTable->lpWSPCloseSocket(s, lpErrno);

			if (s == Common::GetInstance()->m_GameSock)
			{
				Log("Socket closed by application.. (%d). CallAddr: %02x", nRet, _ReturnAddress());
				Common::GetInstance()->m_GameSock = INVALID_SOCKET;
			}

			return nRet;
		}

		/// <summary>
		/// 
		/// </summary>
		INT WSPAPI WSPStartup_Hook(
			WORD				wVersionRequested,
			LPWSPDATA			lpWSPData,
			LPWSAPROTOCOL_INFOW lpProtocolInfo,
			WSPUPCALLTABLE		UpcallTable,
			LPWSPPROC_TABLE		lpProcTable
		)
		{
			int nRet = WSPStartup_Original(wVersionRequested, lpWSPData, lpProtocolInfo, UpcallTable, lpProcTable);

			if (nRet == NO_ERROR)
			{
				Log("Overriding socket routines..");

				Common::GetInstance()->m_GameSock = INVALID_SOCKET;
				Common::GetInstance()->m_pProcTable = lpProcTable;

				lpProcTable->lpWSPConnect = WSPConnect_Hook;
				lpProcTable->lpWSPGetPeerName = WSPGetPeerName_Hook;
				lpProcTable->lpWSPCloseSocket = WSPCloseSocket_Hook;
			}
			else
			{
				Log("WSPStartup Error Code: %d", nRet);
			}

			return nRet;
		}
	}
}
