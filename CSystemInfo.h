//
//! @file CSystemInfo.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CSystemInfo_H
#define _INC_CSystemInfo_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CSingleton.h"
#include "CFilePath.h"

#ifdef __linux__
#include <sys/utsname.h>	// uname()
#endif
UNITTEST_PREDEF(CSystemInfo)

namespace Gray
{
	//! OS build type name. OS target known at compile time. Actually run environment may vary of course.

#ifdef _MFC_VER
#if defined(USE_64BIT)
#define GRAY_BUILD_NAME "WinMFC64"
#else
#define GRAY_BUILD_NAME "WinMFC"
#endif
#elif defined(UNDER_CE)
#define GRAY_BUILD_NAME "WinCE"	// What CPU?
#elif defined(_WIN64)
#define GRAY_BUILD_NAME "Win64"
#elif defined(_WIN32)
#define GRAY_BUILD_NAME "Win32"
#elif defined(_BSD)	// FREEBSD
#define GRAY_BUILD_NAME "FreeBSD"		// Apple/MAC BSD system ? use clang ?
#elif defined(__linux__)
#ifdef USE_64BIT
#define GRAY_BUILD_NAME "Linux64"
#else
#define GRAY_BUILD_NAME "Linux32"
#endif
#else
#error NOOS
#endif

	class GRAYCORE_LINK CSystemInfo : public CSingleton < CSystemInfo >
	{
		//! @class Gray::CSystemInfo
		//! The system as a whole. (as far as we can detect) not just the current running app/process or user login.
		//! The detected system params may be effected by system virtualization.

		friend class CSingleton < CSystemInfo > ;

	protected:
		CStringF m_sSystemName;		//!< Cached (or overridden for debug purposes) system name.

#ifdef _WIN32
	public:
		SYSTEM_INFO m_SystemInfo;	//!< Cached info.  _MSC_VER <= 1200
	protected:
		OSVERSIONINFOEXW m_osInfo;	//!< always use *W version and call RtlGetVersion() to overcome M$ nerf. OSVERSIONINFOEXW
#elif defined(__linux__)
	protected:
		struct utsname m_utsname;	//!< output from uname() on __linux__.
		UINT m_nOSVer;				//!< Major << 8 | minor
		UINT m_nNumberOfProcessors;	//!< should we worry about SMP issues ?
#else
#error NOOS
#endif
#ifndef USE_64BIT
		bool m_bOS64Bit;			//!< Is OS 64 bit? maybe a 32 bit app under a 64 bit OS. _WIN32 WOW ?
#endif

	protected:
		CSystemInfo();
		~CSystemInfo();

	public:
		UINT get_NumberOfProcessors() const;	// SMP issues ?
		bool isOS64Bit() const;

		cString get_OSName() const;
		UINT get_OSVer() const;

#ifdef _WIN32
		bool isOSNTAble() const;
		bool isOSXPAble() const;
#endif
#ifdef __linux__
		bool isVer3_17_plus() const;
#endif

		CStringF get_SystemName();		//!< The node name of the machine.
		static StrLen_t GRAYCALL GetSystemDir(FILECHAR_t* pszDir, StrLen_t iLenMax);
		static CStringF GRAYCALL get_SystemDir();

		static bool GRAYCALL SystemShutdown(bool bReboot);
		static void GRAYCALL SystemBeep();

		CHEAPOBJECT_IMPL;
		UNITTEST_FRIEND(CSystemInfo);
	};
};

#endif // _INC_CSystemInfo_H
