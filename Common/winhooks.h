#pragma once
#include "Common.h"
#include <WinSock2.h>
#include <Windows.h>
#include <WS2spi.h>

#include "winhook_types.h"

#if DIRECTX_VERSION

#if DIRECTX_VERSION == 8
#include "directx/dx8/d3d8.h"
#elif DIRECTX_VERSION == 9
#include <d3d9.h>
#endif

#endif

namespace WinHooks
{
#if DIRECTX_VERSION
	namespace DirectX
	{
		// these parameters are common to any version of directx that we work with
		extern WNDPROC g_pWndProc;
		extern std::function<void()> g_pDrawingFunc;
		extern HWND					g_hWnd;
		extern LPDIRECT3DX             g_pD3D;
		extern IDirect3DDeviceX* g_pd3dDevice;
		extern D3DPRESENT_PARAMETERS   g_d3dpp;

		LRESULT D3D_WndProc_Hook(
			HWND hWnd,
			UINT msg,
			WPARAM wParam,
			LPARAM lParam
		);

		/// <summary>
		/// This is a windows API hook that we use to catch the pointer to D3DX (8 or 9) as MapleStory is
		/// initializing the graphics drivers. This gets called once during startup and the returns the
		/// DX interface pointer that is used to control all of the graphics.
		/// </summary>
		HRESULT WINAPI D3DX__CreateDevice_Hook(
			LPDIRECT3DX pThis,
			UINT Adapter,
			D3DDEVTYPE DeviceType,
			HWND hFocusWindow,
			DWORD BehaviorFlags,
			D3DPRESENT_PARAMETERS* pPresentationParameters,
			IDirect3DDeviceX** ppReturnedDeviceInterface
		);

		HRESULT WINAPI D3DX__EndScene_Hook(LPDIRECT3DDEVICEX pThis);

		IDirect3DX* WINAPI Direct3DCreateX_Hook(
			UINT SDKVersion
		);
	}

#endif

	/// <summary>
	/// Used to map out imports used by MapleStory.
	/// The log output can be used to reconstruct the _ZAPIProcAddress struct
	/// ZAPI struct is the dword before the while loop when searching for aob: 68 FE 00 00 00 ?? 8D
	/// </summary>
	FARPROC WINAPI GetProcAddress_Hook(HMODULE hModule, LPCSTR lpProcName);

	/// <summary>
	/// CreateMutexA is the first Windows library call after the executable unpacks itself.
	/// We hook this function to do all our memory edits and hooks when it's called.
	/// </summary>
	HANDLE WINAPI CreateMutexA_Hook(
		LPSECURITY_ATTRIBUTES lpMutexAttributes,
		BOOL				  bInitialOwner,
		LPCSTR				  lpName
	);

	/// <summary>
	/// In some versions, Maple calls this library function to check if the anticheat has started.
	/// We can spoof this and return a fake handle for it to close.
	/// </summary>
	HANDLE WINAPI OpenMutexA_Hook(
		DWORD  dwDesiredAccess,
		BOOL   bInitialOwner,
		LPCSTR lpName
	);

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
	);

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
		LPPROCESS_INFORMATION lpProcessInformation
	);

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
	);

	/// <summary>
	/// Used to track what processes Maple opens.
	/// </summary>
	HANDLE WINAPI OpenProcess_Hook(
		DWORD dwDesiredAccess,
		BOOL  bInheritHandle,
		DWORD dwProcessId
	);

	/// <summary>
	/// This library call is used by nexon to determine the locale of the connecting clients PC. We spoof it.
	/// </summary>
	/// <returns></returns>
	UINT WINAPI GetACP_Hook(); // AOB: FF 15 ?? ?? ?? ?? 3D ?? ?? ?? 00 00 74 <- library call inside winmain func

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
	);

	/// <summary>
	/// We use this function to track what memory addresses are killing the process.
	/// There are more ways that Maple kills itself, but this is one of them.
	/// </summary>
	LONG NTAPI NtTerminateProcess_Hook(
		HANDLE hProcHandle,
		LONG   ntExitStatus
	);

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
	);

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
		);

		/// <summary>
		/// 
		/// </summary>
		INT WSPAPI WSPGetPeerName_Hook(
			SOCKET			 s,
			struct sockaddr* name,
			LPINT			 namelen,
			LPINT			 lpErrno
		);

		/// <summary>
		/// 
		/// </summary>
		INT WSPAPI WSPCloseSocket_Hook(
			SOCKET s,
			LPINT  lpErrno
		);

		/// <summary>
		/// 
		/// </summary>
		INT WSPAPI WSPStartup_Hook(
			WORD				wVersionRequested,
			LPWSPDATA			lpWSPData,
			LPWSAPROTOCOL_INFOW lpProtocolInfo,
			WSPUPCALLTABLE		UpcallTable,
			LPWSPPROC_TABLE		lpProcTable
		);
	}
}