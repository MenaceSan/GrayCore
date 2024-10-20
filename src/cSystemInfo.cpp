//! @file cSystemInfo.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrArg.h"
#include "StrConst.h"
#include "StrT.h"
#include "cAppState.h"
#include "cOSModule.h"
#include "cSystemHelper.h"
#include "cSystemInfo.h"  // class implemented
#include "cThreadBase.h"  // SleepCurrent

#ifdef __linux__
#include <fcntl.h>
#include <linux/kd.h>
#include <linux/reboot.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>  // gethostname
#elif defined(UNDER_CE)
#include <winsock2.h>  // gethostname
#elif defined(_WIN32)
#include <winioctl.h>  // OSVERSIONINFOEXW
#include <winreg.h>
#else
#error NOOS
#endif

namespace Gray {
cSingleton_IMPL(cSystemInfo);
cSingleton_IMPL(cSystemHelper);

cSystemInfo::cSystemInfo() : cSingleton<cSystemInfo>(this) {
#ifdef _WIN32
    // In Windows 10 the M$ A*holes nerf GetVersionEx(). must call RtlGetVersion().
    // http://www.codeproject.com/Articles/678606/Part-Overcoming-Windows-s-deprecation-of-GetVe?msg=5080848#xx5080848xx
    // GetVersionEx was declared deprecated ?? M$ is crazy. recommended alternatives don't really do what we want.
    // same as ? HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion

    cMem::Zero(&_OsInfo, sizeof(_OsInfo));
    _OsInfo.dwOSVersionInfoSize = sizeof(_OsInfo);

    cOSModule modNT;
    if (modNT.AttachModuleName(_FN("ntdll.dll"), cOSModule::k_Load_Normal)) {
        typedef LONG(WINAPI * tRtlGetVersion)(::OSVERSIONINFOEXW*);
        tRtlGetVersion pfRtlGetVersion = CastFPtrTo<tRtlGetVersion>(modNT.GetSymbolAddress("RtlGetVersion"));
        if (pfRtlGetVersion != nullptr) {  // This will never happen (all processes load ntdll.dll)
            const LONG Status = pfRtlGetVersion(&_OsInfo);
            if (Status != 0) {  // STATUS_SUCCESS;
                // must be an old version of windows. win95 or win31 ? Should NOT happen!
                pfRtlGetVersion = nullptr;
                // DEBUG_WARN((""));
            }
        }
    }

    ::GetSystemInfo(&_SystemInfo);

#if !defined(USE_64BIT) && !defined(UNDER_CE)
    BOOL bWow6432 = false;
    ::IsWow64Process(::GetCurrentProcess(), &bWow6432);
    _isOS64Bit = bWow6432;
#endif

#elif defined(__linux__)
    // sysinfo()

    int iRet = ::uname(&_UtsName);
    if (iRet < 0) {
        // invalid !
        cMem::Zero(&_UtsName, sizeof(_UtsName));
    }

    // _UtsName.release = "4.4.3-201.fc22.x86_64"
    _nOSVer = (StrT::toU(_UtsName.release) << 8);
    const char* pszVer2 = StrT::FindChar(_UtsName.release, '.');
    if (pszVer2 != nullptr) {
        _nOSVer |= StrT::toU(pszVer2 + 1);
    }

    _nPageSize = cMem::k_PageSizeMin;

#if !defined(USE_64BIT)
    // file /sbin/init // uname -a // _UtsName.machine
    _isOS64Bit = (StrT::FindStrI(_UtsName.release, "x86_64") != nullptr);
#endif

    // _UtsName.version= "#1 SMP Sat Feb 27 11:53:28 UTC 2016"
    // TODO: NR_CPUS Linux
    // num_online_cpus();
    // read /proc/cpuinfo
    _nNumberOfProcessors = ::sysconf(_SC_NPROCESSORS_ONLN);
    // _nNumberOfProcessors = (StrT::FindStrI(_UtsName.version, "SMP") != nullptr) ? 2 : 1;

    // physical id : 0
    // core id : 1
    // cpu cores : 2
    // If you have two CPUs in cpuinfo with the same physical id and core id, then we are hyperthreading!
    // When you have all CPUs with different physical ids, than you have SMP,
    // and when you have CPUs with one physical id and different core ids - you have one multicore CPU.

#else
#error NOOS
#endif

    ASSERT(get_PageSize() >= cMem::k_PageSizeMin);
}

UINT cSystemInfo::get_NumberOfProcessors() const noexcept {
    //! Is SMP an issue? Do i need to worry about multiple processors ?
    //! Creating more worker threads than CPU's might not help you if its a CPU heavy task.
#ifdef _WIN32
    return _SystemInfo.dwNumberOfProcessors;
#else  // __linux__
    return _nNumberOfProcessors;
#endif
}

bool cSystemInfo::isOS64Bit() const noexcept {
    //! can both the OS and CPU handle 64 bit apps.
    //! A 32 bit app can run on 64 bit system.
#ifdef USE_64BIT
    return true;  // I can only run on 64 bit OS so it must be.
#else
    return _isOS64Bit;
#endif
}

UINT cSystemInfo::get_OSVer() const noexcept {
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
    return _OsInfo.dwMajorVersion << 8 | _OsInfo.dwMinorVersion;
#else
    return _nOSVer;
#endif
}

size_t cSystemInfo::get_PageSize() const noexcept {
    // cMem::k_PageSizeMin
#ifdef _WIN32
    return _SystemInfo.dwPageSize;
#else
    return _nPageSize;
#endif
}

#ifdef _WIN32
bool cSystemInfo::isOSNTAble() const noexcept {
    //! Does this OS support NT services ? _WIN32 only of course.
    if (_OsInfo.dwPlatformId < VER_PLATFORM_WIN32_NT) return false;
    return (this->get_OSVer() >= 0x400);
}

bool cSystemInfo::isOSXPAble() const noexcept {
    //! Does this OS have XP Dll's. _WIN32 only of course.
    if (_OsInfo.dwPlatformId < VER_PLATFORM_WIN32_NT) return false;
    return (this->get_OSVer() >= 0x501);
}
#endif

#ifdef __linux__
bool cSystemInfo::isVer3_17_plus() const noexcept {
    //! is version at least 3.17.0 or greater.
    return (this->get_OSVer() >= 0x0311);
}
#endif

StrLen_t GRAYCALL cSystemInfo::GetSystemDir(cSpanX<FILECHAR_t> ret) {  // static
    //! HRESULT hRes = _FNF(::SHGetFolderPath)( g_MainFrame.GetSafeHwnd(), CSIDL_SYSTEM, nullptr, 0, szPath );

#ifdef UNDER_CE
    return StrT::Copy(ret, _FN("\\Windows"));
#elif defined(_WIN32)
    // GetWindowsDirectory() = "C:\Windows"
    // GetSystemDirectory() == "C:\Windows\System32"
    return (StrLen_t)_FNF(::GetSystemDirectory)(ret.get_PtrWork(), ret.get_MaxLen());  // "C:\Windows\System32"
#elif defined(__linux__)
    // NOT the same as GetEnv("topdir");
    return StrT::Copy(ret, _FN("/sbin"));
#else
#error NOOS
#endif
}

HRESULT GRAYCALL cSystemInfo::GetSystemName(cSpanX<FILECHAR_t> ret) {  // static
    // HResult::GetLast() if this fails ?
    ret.get_PtrWork()[0] = '\0';
#if defined(__linux__) || defined(UNDER_CE)
    int iErrNo = ::gethostname(ret.get_PtrWork(), ret.get_Count());
    if (iErrNo != 0) return HResult::FromPOSIX(iErrNo);  // SOCKET_ERROR
    return StrT::Len(ret.get_PtrConst());
#elif defined(_WIN32)
    DWORD dwSize = CastN(DWORD, ret.get_Count());  // size in TCHARs
    if (!_FNF(::GetComputerName)(ret.get_PtrWork(), &dwSize)) return HResult::GetLastDef();
    return CastN(StrLen_t, dwSize);
#else
#error NOOS
#endif
}

bool GRAYCALL cSystemInfo::SystemShutdown(bool bReboot) {  // static
    //! Shut down or reboot the whole system. not just log off the user or close the app.

#ifdef UNDER_CE
    return false;
#elif defined(_WIN32)
#ifdef _MSC_VER
    return _GTN(::InitiateSystemShutdownEx)(nullptr,  // lpMachineName
                                            _GT("App requested shutdown"),
                                            cTimeSys::k_INF,  // timeout
                                            true,             // __in bool bForceAppsClosed
                                            bReboot,          // bRebootAfterShutdown
                                            SHTDN_REASON_FLAG_PLANNED);
#else
    return _GTN(::InitiateSystemShutdown)(nullptr,  // lpMachineName
                                          (LPSTR)_GT("App requested shutdown"),
                                          cTimeSys::k_INF,  // timeout
                                          true,             // __in bool bForceAppsClosed
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

void GRAYCALL cSystemInfo::SystemBeep() {  // static
    // like _WIN32 MessageBeep(), not Beep() (which can get redirected if RDP)
#if defined(UNDER_CE)
    ::MessageBeep(0xFFFFFFFF);
#elif defined(_WIN32)
#if 1
    ::MessageBeep(0xFFFFFFFF);
#else

    const int k_nFrequency = 2000;  // freq in hz
    const int k_nDuration = 1000;   // len in ms =  TIMESYS_t

    // http://www.rohitab.com/discuss/topic/19467-how-do-i-make-the-computer-beep-using-c/
    // DeviceIoControl IOCTL_BEEP_SET BEEP_SET_PARAMETERS on "\Device\Beep" DD_BEEP_DEVICE_NAME
#define IOCTL_BEEP_SET CTL_CODE(FILE_DEVICE_BEEP, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
    typedef struct _BEEP_SET_PARAMETERS {
        ULONG Frequency;
        ULONG Duration;
    } BEEP_SET_PARAMETERS;

    bool bReturn = ::DefineDosDeviceA(DDD_RAW_TARGET_PATH, "DosBeep", "\\Device\\Beep");  // _FNF ?

    cOSHandle hBeep(::CreateFileW(__TOW(FILEDEVICE_PREFIX) __TOW("DosBeep"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE));
    if (!hBeep.isValidHandle()) {
        HRESULT hRes = HResult::GetLast();
        return;
    }
    BEEP_SET_PARAMETERS oBSP;
    oBSP.Frequency = k_nFrequency;  // Hz
    oBSP.Duration = k_nDuration;    // TIMESYS_t

    DWORD nBytesReturned = 0;
    HRESULT hRes = ::DeviceIoControl(hBeep, IOCTL_BEEP_SET, &oBSP, sizeof(oBSP), nullptr, 0, &nBytesReturned, nullptr);
    if (FAILED(hRes)) return;
    if ((oBSP.Frequency != 0x0 || oBSP.Duration != 0x0) && oBSP.Duration != (DWORD)-1) {
        ::SleepEx(oBSP.Duration, true);
    }
#endif

#elif defined(__linux__)

    const int k_nFrequency = 2000;  // freq in hz
    const int k_nDuration = 1000;   // len in ms =  TIMESYS_t

    cOSHandle hBeep;
    hBeep.OpenHandle("/dev/console", O_WRONLY);
    hBeep.IOCtl(KIOCSOUND, (int)(1193180 / k_nFrequency));
    cThreadId::SleepCurrent(k_nDuration);
    hBeep.IOCtl(KIOCSOUND, 0);
#else
#error NOOS
#endif
}

//**********************************************

cSystemHelper::cSystemHelper() : cSingleton<cSystemHelper>(this), _SysInfo(cSystemInfo::I()) {}

cString cSystemHelper::get_OSInfoStr() const {
    //! More detailed info about the actual OS we are running on.
    //! like GRAY_BUILD_NAME but dynamic.
    //! number of processors and processor type etc.
    //! Do we support MMX etc ?
    //! http://sourceforge.net/p/predef/wiki/Architectures/

    cString sTmp;
#ifdef _WIN32
    const GChar_t* pszCPU;
    switch (_SysInfo._SystemInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_INTEL:
            switch (LOBYTE(_SysInfo._SystemInfo.wProcessorLevel)) {
                case 3:
                    pszCPU = _GT("i386");
                    break;
                case 4:
                    pszCPU = _GT("i486");
                    break;
                case 5:
                    pszCPU = _GT("Pentium");
                    break;
                case 6:
                    pszCPU = _GT("Pentium Pro");
                    break;  // /II/III/IV
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
#if defined(_M_ARM) || defined(_M_ARMT) || defined(__arm__) || defined(__thumb__)  // VC and GNUC
        case PROCESSOR_ARCHITECTURE_ARM:
            pszCPU = _GT("ARM");
            break;
#endif

#if 0  // we would have to compile special for this !
		case PROCESSOR_ARCHITECTURE_MIPS:
			pszCPU = _GT("Mips");
			break;
		case PROCESSOR_ARCHITECTURE_PPC:
			pszCPU = _GT("PPC");
			break;
		case PROCESSOR_ARCHITECTURE_ALPHA:
			switch (_SystemInfo.wProcessorLevel)
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
    switch (_SysInfo._OsInfo.dwPlatformId) {
        case VER_PLATFORM_WIN32_NT:
            pszOS = _GT("NT?");
            switch (_SysInfo._OsInfo.dwMajorVersion) {
                case 4:
                    pszOS = _GT("NT");
                    break;
                case 5:
                    switch (_SysInfo._OsInfo.dwMinorVersion) {
                        case 0:
                            pszOS = _GT("2000");
                            break;
                        case 1:
                            pszOS = _GT("XP");
                            break;
                        case 2:  // XP 64 bit ? or Home server ?
                        default:
                            pszOS = _GT("Server 2003");
                            break;
                    }
                    break;
                case 6:
                    switch (_SysInfo._OsInfo.dwMinorVersion) {
                        case 0:
                            pszOS = _GT("Vista");
                            break;
                        case 1:
                            pszOS = _GT("7");
                            break;  // 6.1 = Windows 7
                        case 2:
                            pszOS = _GT("8");
                            break;  // 6.2 = Windows 8
                        default:
                            pszOS = _GT("8?");
                            break;
                    }
                    break;
                case 10:
                    pszOS = _GT("10");  // 11 is same ?
                    break;                  
            }
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            pszOS = _GT("Win9X/ME");
            break;
        default:
            pszOS = _GT("Unknown");
            break;
    }

    sTmp.Format(_GT("Windows %s %dbit v%d.%d.%d (%d %s CPU%s)"), StrArg<GChar_t>(pszOS), _SysInfo.isOS64Bit() ? 64 : 32, _SysInfo._OsInfo.dwMajorVersion, _SysInfo._OsInfo.dwMinorVersion, _SysInfo._OsInfo.dwBuildNumber, _SysInfo.get_NumberOfProcessors(),
                StrArg<GChar_t>(pszCPU), StrArg<GChar_t>((_SysInfo.get_NumberOfProcessors() > 1) ? _GT("s") : _GT("")));
#else  // __linux__
    if (StrT::IsWhitespace(_SysInfo._UtsName.sysname)) {
        return _GT("Linux UNK VER!?");
    }

    // NOTE: ignore _UtsName.nodename here since it is the same as SystemName
    sTmp.Format(_GT("%s %dbit '%s' '%s' '%s'"), StrArg<GChar_t>(&_SysInfo._UtsName.sysname[0]), _SysInfo.isOS64Bit() ? 64 : 32, StrArg<GChar_t>(&_SysInfo._UtsName.release[0]), StrArg<GChar_t>(&_SysInfo._UtsName.version[0]),
                StrArg<GChar_t>(&_SysInfo._UtsName.machine[0]));
#endif
    return sTmp;
}

cStringF cSystemHelper::get_SystemName() {
    //! Get this computers station name.
    //! UNDER_CE -> "HKLM\Ident\Name"

    if (!_sSystemName.IsEmpty()) {
        return _sSystemName;
    }

#if defined(__linux__) || defined(UNDER_CE)
    const StrLen_t kSizeSystemName = cFilePath::k_MaxLen;
#elif defined(_WIN32)
    const StrLen_t kSizeSystemName = MAX_COMPUTERNAME_LENGTH;
#else
#error NOOS
#endif

    FILECHAR_t szNodeName[kSizeSystemName + 2];
    HRESULT hRes = cSystemInfo::GetSystemName(TOSPAN(szNodeName));
    if (FAILED(hRes)) {
        // NO NAME? I've seen this fail on some machines. don't know why.
        return "";
    }

    _sSystemName = szNodeName;
    return _sSystemName;
}

cFilePath GRAYCALL cSystemHelper::get_SystemDir() {  // static
    //! Where does the OS keep its files. CSIDL_SYSTEM
    //! GetSystemDirectory() == "C:\Windows\System32" or "C:\Windows\SysWOW64" for 32 bit app on 64 bit OS.
    FILECHAR_t szTmp[cFilePath::k_MaxLen];
    cSystemInfo::GetSystemDir(TOSPAN(szTmp));
    return szTmp;
}
}  // namespace Gray
