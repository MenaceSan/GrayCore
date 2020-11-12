//
//! @file cSystemInfo.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cSystemInfo.h"   // class implemented
#include "cSystemHelper.h"
#include "cAppState.h"
#include "StrT.h"
#include "StrConst.h"
#include "StrArg.h"
#include "cOSModule.h"

#ifdef __linux__
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h> // gethostname
#include <fcntl.h>
#include <linux/reboot.h>
#include <linux/kd.h>
#elif defined(UNDER_CE)
#include <winsock2.h> // gethostname
#elif defined(_WIN32)
#include <winreg.h>
#include <winioctl.h>	// OSVERSIONINFOEXW
#else
#error NOOS
#endif

namespace Gray
{
	cSystemInfo::cSystemInfo()
		: cSingleton<cSystemInfo>(this, typeid(cSystemInfo))
	{
#ifdef _WIN32
		// Windows 10 nerfs GetVersionEx(). must call RtlGetVersion(). M$ A*holes.
		// http://www.codeproject.com/Articles/678606/Part-Overcoming-Windows-s-deprecation-of-GetVe?msg=5080848#xx5080848xx
		// GetVersionEx was declared deprecated ?? M$ is crazy. recommended alternatives don't really do what we want.

		cMem::Zero(&m_OsInfo, sizeof(m_OsInfo));
		m_OsInfo.dwOSVersionInfoSize = sizeof(m_OsInfo);

		cOSModule modNT;
		if (modNT.AttachModuleName(_FN("ntdll.dll")))
		{
			typedef LONG(WINAPI* tRtlGetVersion)(OSVERSIONINFOEXW*);
			tRtlGetVersion pfRtlGetVersion = (tRtlGetVersion)modNT.GetSymbolAddress("RtlGetVersion");
			if (pfRtlGetVersion != nullptr)	// This will never happen (all processes load ntdll.dll)
			{
				LONG Status = pfRtlGetVersion(&m_OsInfo);
				if (Status != 0) // STATUS_SUCCESS;
				{
					// must be an old version of windows. win95 or win31 ? Should NOT happen!
					pfRtlGetVersion = nullptr;
				}
			}
		}

		::GetSystemInfo(&m_SystemInfo);

#if ! defined(USE_64BIT) && ! defined(UNDER_CE)
		BOOL bWow6432 = false;
		::IsWow64Process(::GetCurrentProcess(), &bWow6432);
		m_bOS64Bit = bWow6432;
#endif

#elif defined(__linux__)
		// sysinfo()

		int iRet = ::uname(&m_utsname);
		if (iRet < 0)
		{
			// invalid !
			cMem::Zero(&m_utsname, sizeof(m_utsname));
		}

		// m_utsname.release = "4.4.3-201.fc22.x86_64"
		m_nOSVer = (StrT::toU(m_utsname.release) << 8);
		const char* pszVer2 = StrT::FindChar(m_utsname.release, '.');
		if (pszVer2 != nullptr)
		{
			m_nOSVer |= StrT::toU(pszVer2 + 1);
		}

#if ! defined(USE_64BIT) 
		// file /sbin/init // uname -a // m_utsname.machine
		m_bOS64Bit = (StrT::FindStrI(m_utsname.release, "x86_64") != nullptr);
#endif

		// m_utsname.version= "#1 SMP Sat Feb 27 11:53:28 UTC 2016"
		// TODO: NR_CPUS Linux
		// num_online_cpus();
		// read /proc/cpuinfo
		m_nNumberOfProcessors = ::sysconf(_SC_NPROCESSORS_ONLN);
		// m_nNumberOfProcessors = (StrT::FindStrI(m_utsname.version, "SMP") != nullptr) ? 2 : 1;

		// physical id : 0
		// core id : 1
		// cpu cores : 2
		// If you have two CPUs in cpuinfo with the same physical id and core id, then we are hyperthreading! 
		// When you have all CPUs with different physical ids, than you have SMP, 
		// and when you have CPUs with one physical id and different core ids - you have one multicore CPU.

#else
#error NOOS
#endif
	}

	cSystemInfo::~cSystemInfo()
	{
	}

	UINT cSystemInfo::get_NumberOfProcessors() const
	{
		//! Is SMP an issue? Do i need to worry about multiple processors ?
		//! Creating more worker threads than CPU's might not help you if its a CPU heavy task.
#ifdef _WIN32
		return m_SystemInfo.dwNumberOfProcessors;
#else // __linux__
		return m_nNumberOfProcessors;
#endif
	}

	bool cSystemInfo::isOS64Bit() const
	{
		//! can both the OS and CPU handle 64 bit apps.
		//! A 32 bit app can run on 64 bit system.
#ifdef USE_64BIT
		return true;	// I can only run on 64 bit OS so it must be.
#else
		return m_bOS64Bit;
#endif
	}

	UINT cSystemInfo::get_OSVer() const
	{
		//! from http://msdn.microsoft.com/en-us/library/ms724429(VS.85).aspx
		//! @return
		//!  >= 0x600 = Vista
		//! 4.0 = Windows NT
		//! 5.0 = Windows 2000
		//! 5.1 = Windows XP (Pro or Home)
		//! 5.2 = Windows Server 2003 R2 or Home Server or Storage Server 2003
		//! 6.0 = Windows Vista or Server 2008 R1
		//! 6.1 = Windows 7 or Server 2008 R2
		//! 6.2 = Windows 8 or Server 2012

#ifdef _WIN32
		return(m_OsInfo.dwMajorVersion << 8 | m_OsInfo.dwMinorVersion);
#else
		return m_nOSVer;
#endif
	}

#ifdef _WIN32
	bool cSystemInfo::isOSNTAble() const
	{
		//! Does this OS support NT services ? _WIN32 only of course.
		if (m_OsInfo.dwPlatformId < VER_PLATFORM_WIN32_NT)
			return false;
		return(this->get_OSVer() >= 0x400);
	}

	bool cSystemInfo::isOSXPAble() const
	{
		//! Does this OS have XP Dll's. _WIN32 only of course.
		if (m_OsInfo.dwPlatformId < VER_PLATFORM_WIN32_NT)
			return false;
		return(this->get_OSVer() >= 0x501);
	}
#endif

#ifdef __linux__
	bool cSystemInfo::isVer3_17_plus() const
	{
		//! is version at least 3.17.0 or greater.
		return(this->get_OSVer() >= 0x0311);
	}
#endif

	StrLen_t GRAYCALL cSystemInfo::GetSystemDir(FILECHAR_t* pszDir, StrLen_t iLenMax) // static
	{
		//! Where does the OS keep its files. CSIDL_SYSTEM
		//! HRESULT hRes = _FNF(::SHGetFolderPath)( g_MainFrame.GetSafeHwnd(), CSIDL_SYSTEM, nullptr, 0, szPath );

#ifdef UNDER_CE
		return StrT::CopyLen(pszDir, _FN("\\Windows"), iLenMax);
#elif defined(_WIN32)
	// GetWindowsDirectory() = "C:\Windows"
	// GetSystemDirectory() == "C:\Windows\System32"
		return (StrLen_t)_FNF(::GetSystemDirectory)(pszDir, iLenMax); // "C:\Windows\System32"
#elif defined(__linux__)
	// NOT the same as GetEnv("topdir");
		return StrT::CopyLen(pszDir, _FN("/sbin"), iLenMax);
#else
#error NOOS
#endif
	}

	HRESULT GRAYCALL cSystemInfo::GetSystemName(FILECHAR_t* pszName, StrLen_t iLenMax) // static
	{
		// HResult::GetLast() if this fails ?

		pszName[0] = '\0';
#if defined(__linux__) || defined(UNDER_CE)
		int iErrNo = ::gethostname(pszName, iLenMax);
		if (iErrNo != 0) // SOCKET_ERROR
			return HResult::FromPOSIX(iErrNo);
		return StrT::Len(pszName);
#elif defined(_WIN32)
		DWORD dwSize = iLenMax;	// size in TCHARs
		if (!_FNF(::GetComputerName)(pszName, &dwSize))	
			return HResult::GetLastDef();
		return (StrLen_t)dwSize;
#else
#error NOOS
#endif	
	}

	bool GRAYCALL cSystemInfo::SystemShutdown(bool bReboot) // static
	{
		//! Shut down or reboot the whole system. not just log off the user or close the app.

#ifdef UNDER_CE
		return false;
#elif defined(_WIN32)
#ifdef _MSC_VER
		return _GTN(::InitiateSystemShutdownEx)(nullptr,	// lpMachineName
			_GT("App requested shutdown"),
			cTimeSys::k_INF,	// timeout
			true,	// __in bool bForceAppsClosed
			bReboot,	// bRebootAfterShutdown
			SHTDN_REASON_FLAG_PLANNED);
#else
		return _GTN(::InitiateSystemShutdown)(nullptr,	// lpMachineName
			(LPSTR)_GT("App requested shutdown"),
			cTimeSys::k_INF,	// timeout
			true,	// __in bool bForceAppsClosed
			bReboot);
#endif
#elif defined(__linux__)
	// shutdown -h now = power down
	// shutdown -r now = reboot.
	// int reboot(int magic, int magic2, int  flag, void *arg);

		return false;
#else
#error NOOS
#endif
	}

	void GRAYCALL cSystemInfo::SystemBeep() // static
	{
		// like _WIN32 MessageBeep(), not Beep() (which can get redirected if RDP)
#if defined(UNDER_CE)
		::MessageBeep(0xFFFFFFFF);
#elif defined(_WIN32)
#if 1
		::MessageBeep(0xFFFFFFFF);
#else

		const int k_nFrequency = 2000; // freq in hz
		const int k_nDuration = 1000; // len in ms =  TIMESYS_t

		// http://www.rohitab.com/discuss/topic/19467-how-do-i-make-the-computer-beep-using-c/
		// DeviceIoControl IOCTL_BEEP_SET BEEP_SET_PARAMETERS on "\Device\Beep" DD_BEEP_DEVICE_NAME
#define IOCTL_BEEP_SET CTL_CODE(FILE_DEVICE_BEEP, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
		typedef struct _BEEP_SET_PARAMETERS
		{
			ULONG Frequency;
			ULONG Duration;
		} BEEP_SET_PARAMETERS;

		bool bReturn = ::DefineDosDeviceA(DDD_RAW_TARGET_PATH, "DosBeep", "\\Device\\Beep");	// _FNF ?

		cOSHandle hBeep(::CreateFileW(__TOW(FILEDEVICE_PREFIX) __TOW("DosBeep"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE));
		if (!hBeep.isValidHandle())
		{
			HRESULT hRes = HResult::GetLast();
			return;
		}
		BEEP_SET_PARAMETERS oBSP;
		oBSP.Frequency = k_nFrequency;	// Hz
		oBSP.Duration = k_nDuration;	// TIMESYS_t

		DWORD nBytesReturned = 0;
		HRESULT hRes = ::DeviceIoControl(hBeep, IOCTL_BEEP_SET, &oBSP, sizeof(oBSP), nullptr, 0, &nBytesReturned, nullptr);
		if (FAILED(hRes))
			return;
		if ((oBSP.Frequency != 0x0 || oBSP.Duration != 0x0) && oBSP.Duration != (DWORD)-1)
		{
			::SleepEx(oBSP.Duration, true);
		}
#endif

#elif defined(__linux__)

		const int k_nFrequency = 2000; // freq in hz
		const int k_nDuration = 1000; // len in ms =  TIMESYS_t

		cOSHandle hBeep;
		hBeep.OpenHandle("/dev/console", O_WRONLY);
		hBeep.IOCtl(KIOCSOUND, (int)(1193180 / k_nFrequency));
		::usleep(1000 * k_nDuration);
		hBeep.IOCtl(KIOCSOUND, 0);
#else
#error NOOS
#endif
	}

	//**********************************************

	cSystemHelper::cSystemHelper()
		: cSingleton<cSystemHelper>(this, typeid(cSystemHelper))
		, m_Info(cSystemInfo::I())
	{
	}

	cString cSystemHelper::get_OSInfoStr() const
	{
		//! More detailed info about the actual OS we are running on.
		//! like GRAY_BUILD_NAME but dynamic.
		//! number of processors and processor type etc.
		//! Do we support MMX etc ?
		//! http://sourceforge.net/p/predef/wiki/Architectures/

		cString sTmp;
#ifdef _WIN32
		const GChar_t* pszCPU;
		switch (m_Info.m_SystemInfo.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_INTEL:
			switch (LOBYTE(m_Info.m_SystemInfo.wProcessorLevel))
			{
			case 3: pszCPU = _GT("i386"); break;
			case 4: pszCPU = _GT("i486"); break;
			case 5: pszCPU = _GT("Pentium"); break;
			case 6: pszCPU = _GT("Pentium Pro"); break; // /II/III/IV
			default:
				pszCPU = _GT("x86");
				break;
			}
			break;

#ifndef UNDER_CE
		case PROCESSOR_ARCHITECTURE_AMD64:
			pszCPU = _GT("AMD64");
			break;
#endif
#if defined(_M_ARM) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__)	// VC and GNUC
		case PROCESSOR_ARCHITECTURE_ARM:
			pszCPU = _GT("ARM");
			break;
#endif

#if 0	// we would have to compile special for this !
		case PROCESSOR_ARCHITECTURE_MIPS:
			pszCPU = _GT("Mips");
			break;
		case PROCESSOR_ARCHITECTURE_PPC:
			pszCPU = _GT("PPC");
			break;
		case PROCESSOR_ARCHITECTURE_ALPHA:
			switch (m_SystemInfo.wProcessorLevel)
			{
			case 21064: pszCPU = _GT("Alpha 21064"); break;
			case 21066: pszCPU = _GT("Alpha 21066"); break;
			case 21164: pszCPU = _GT("Alpha 21164"); break;
			default:
				pszCPU = _GT("Alpha");
				break;
			}
			break;
#endif
		default:
			pszCPU = _GT("?");
			break;
		}

		const GChar_t* pszOS;
		switch (m_Info.m_OsInfo.dwPlatformId)
		{
		case VER_PLATFORM_WIN32_NT:
			pszOS = _GT("NT?");
			switch (m_Info.m_OsInfo.dwMajorVersion)
			{
			case 4: pszOS = _GT("NT"); break;
			case 5:
				switch (m_Info.m_OsInfo.dwMinorVersion)
				{
				case 0: pszOS = _GT("2000"); break;
				case 1: pszOS = _GT("XP"); break;
				case 2:	// XP 64 bit ? or Home server ?
				default: pszOS = _GT("Server 2003"); break;
				}
				break;
			case 6:
				switch (m_Info.m_OsInfo.dwMinorVersion)
				{
				case 0: pszOS = _GT("Vista"); break;
				case 1: pszOS = _GT("7"); break;	// 6.1 = Windows 7
				case 2: pszOS = _GT("8"); break;	// 6.2 = Windows 8
				default: pszOS = _GT("8?"); break;
				}
				break;
			case 10:
				pszOS = _GT("10"); break;
				break;
			}
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			pszOS = _GT("Win9X/ME"); break;
		default:
			pszOS = _GT("Unknown"); break;
		}

		sTmp.Format(_GT("Windows %s %dbit v%d.%d.%d (%d %s CPU%s)"),
			StrArg<GChar_t>(pszOS),
			m_Info.isOS64Bit() ? 64 : 32,
			m_Info.m_OsInfo.dwMajorVersion,
			m_Info.m_OsInfo.dwMinorVersion,
			m_Info.m_OsInfo.dwBuildNumber,
			m_Info.get_NumberOfProcessors(),
			StrArg<GChar_t>(pszCPU),
			StrArg<GChar_t>((m_Info.get_NumberOfProcessors() > 1) ? _GT("s") : _GT("")));
#else // __linux__
		if (StrT::IsWhitespace(m_Info.m_utsname.sysname))
		{
			return _GT("Linux UNK VER!?");
		}

		// NOTE: ignore m_utsname.nodename here since it is the same as SystemName
		sTmp.Format(_GT("%s %dbit '%s' '%s' '%s'"),
			StrArg<GChar_t>(&m_Info.m_utsname.sysname[0]),
			m_Info.isOS64Bit() ? 64 : 32,
			StrArg<GChar_t>(&m_Info.m_utsname.release[0]),
			StrArg<GChar_t>(&m_Info.m_utsname.version[0]),
			StrArg<GChar_t>(&m_Info.m_utsname.machine[0]));
#endif
		return sTmp;
	}

	cStringF cSystemHelper::get_SystemName()
	{
		//! Get this computers station name.
		//! UNDER_CE -> "HKLM\Ident\Name"

		if (!m_sSystemName.IsEmpty())
		{
			return m_sSystemName;
		}

#if defined(__linux__) || defined(UNDER_CE)
		const StrLen_t kSizeSystemName = _MAX_PATH;
#elif defined(_WIN32)
		const StrLen_t kSizeSystemName = MAX_COMPUTERNAME_LENGTH;
#else
#error NOOS
#endif

		FILECHAR_t szNodeName[kSizeSystemName + 1];
		HRESULT hRes = cSystemInfo::GetSystemName(szNodeName, kSizeSystemName);
		if (FAILED(hRes))
		{
			// NO NAME? I've seen this fail on some machines. don't know why.
			return "";
		}

		m_sSystemName = szNodeName;
		return m_sSystemName;
	}

	cStringF GRAYCALL cSystemHelper::get_SystemDir() // static
	{
		//! Where does the OS keep its files. CSIDL_SYSTEM
		//! GetSystemDirectory() == "C:\Windows\System32" or "C:\Windows\SysWOW64" for 32 bit app on 64 bit OS.
		FILECHAR_t szTmp[_MAX_PATH];
		cSystemInfo::GetSystemDir(szTmp, STRMAX(szTmp));
		return szTmp;
	}

}

//******************************************************************

#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cLogMgr.h"

UNITTEST_CLASS(cSystemInfo)
{
	UNITTEST_METHOD(cSystemInfo)
	{
		cSystemHelper& i = cSystemHelper::I();

		cStringF sSysName = i.get_SystemName();
		UNITTEST_TRUE(!sSysName.IsEmpty());

		cString sOsName = i.get_OSInfoStr();
		UNITTEST_TRUE(!sOsName.IsEmpty());

		UINT uOSVer = i.m_Info.get_OSVer();
#ifdef _WIN32
		UNITTEST_TRUE(uOSVer > 0x500);	// For windows.
#endif
#ifdef __linux__
		UNITTEST_TRUE(uOSVer > 0);	// For __linux__.
#endif
		cSystemInfo::SystemBeep();
	}
};
UNITTEST_REGISTER(cSystemInfo, UNITTEST_LEVEL_Core);
#endif
