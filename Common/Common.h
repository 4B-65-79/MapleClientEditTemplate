#pragma once
#include <functional>

/*
 * Define pre-compiler macros here
 */
#define MAPLE_INJECT_USE_IJL false
#define DIRECTX_VERSION 9 // currently only dx9 works

/* forward declarations */
using DWORD = unsigned long;
using UINT_PTR = unsigned int;
using SOCKET = UINT_PTR;
using WSPPROC_TABLE = struct _WSPPROC_TABLE;
class FakeModule;

/// <summary>
/// 
/// </summary>
class Common
{
private:
	struct Config
	{
	private:
		struct WinHooks
		{
			/* define toggles for logging and other behavior separately */
		public:
			bool ImGui_Enable; // this will hook dx9 and wndproc
			bool OpenMutexA_Spoof;

			bool WSPConnect_Logging;
			bool NtTerminateProc_Logging;
			bool OpenProcess_Logging;
			bool CreateProcess_Logging;
			bool OpenMutexA_Logging;
			bool RegCreateKeyA_Logging;
			bool GetProcAddress_Logging;

			WinHooks()
			{
				ImGui_Enable = true;

				OpenMutexA_Spoof = false;
				WSPConnect_Logging = false;
				NtTerminateProc_Logging = false;
				OpenProcess_Logging = false;
				CreateProcess_Logging = false;
				OpenMutexA_Logging = false;
				RegCreateKeyA_Logging = false;
				GetProcAddress_Logging = false;
			}
		};
	public:
		const char* DllName = "LEN.dll";
		const char* MapleExeName = "MapleStory.exe";
		const char* MapleStartupArgs = "";// " GameLaunching 127.0.0.1 8484";

		const char* MapleExitWindowWebUrl = "http";
		const char* MapleWindowClass = "MapleStoryClass";
		const char* MaplePatcherClass = "StartUpDlgClass";
		const char* MapleMutex = "WvsClientMtx";

		DWORD LocaleSpoofValue;
		DWORD SleepAfterUnpackDuration;

		bool  ForceWindowedOnStart;
		bool  InjectImmediately;
		bool  AllowMulticlient;


		Common::Config::WinHooks HookToggleInfo;

		Config()
		{
			HookToggleInfo = WinHooks();

			LocaleSpoofValue = 0;
			SleepAfterUnpackDuration = 0;
			ForceWindowedOnStart = true;
			InjectImmediately = true;
			AllowMulticlient = false;
		}
	};

	static Common* _s_pInstance;
	static Common::Config* _s_pConfig;

public: // public because all the C-style hooks have to access these members
	const char* m_sRedirectIP;
	const char* m_sOriginalIP;

	/* TODO throw all the winsock stuff into its own class */
	SOCKET			m_GameSock;
	WSPPROC_TABLE* m_pProcTable;
	DWORD			m_dwGetProcRetAddr;
	bool			m_bThemidaUnpacked;
	FakeModule* m_pFakeHsModule;

	/// <summary>
	/// Gets called when mutex hook is triggered.
	/// </summary>
	std::function<void()> m_PostMutexFunc;

private: // forcing the class to only have one instance, created through CreateInstance
	Common(bool bHookWinLibs, std::function<void()> const& pPostMutexFunc, const char* sIP, const char* sOriginalIP);

public:
	~Common();
	Common() = delete;
	Common(const Common&) = delete;
	Common& operator =(const Common&) = delete;

	/// <summary>
	/// Function called from library hooks.
	/// Most of the time this should be triggered by the Mutex hook, however, in the case that
	/// the Mutex hook does not get triggered then this will be executed by CreateWindowExA
	/// for redundancy. The contents of this function will only be executed once, even if both 
	/// Mutex and CreateWindow hooks are called properly.
	/// </summary>
	void OnThemidaUnpack();
	static void SetImGuiDrawingFunc(std::function<void()> pDrawingFunc);

	static void CreateInstance(bool bHookWinLibs, std::function<void()> const& pMutexFunc, const char* sIP, const char* sOriginalIP)
	{
		if (_s_pInstance) return;

		_s_pInstance = new Common(bHookWinLibs, pMutexFunc, sIP, sOriginalIP);
	}

	static Common* GetInstance()
	{
		return _s_pInstance;
	}

	static Config* GetConfig()
	{
		if (!_s_pConfig) _s_pConfig = new Config();

		return _s_pConfig;
	}
};

