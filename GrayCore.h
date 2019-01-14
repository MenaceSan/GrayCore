//
//! @file GrayCore.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//! Can be included from an .RC file. RC_INVOKED

#ifndef _INC_GrayCore_H
#define _INC_GrayCore_H	0x002	//!< 0.0.2 Version stamp the API. Especially the CVariant.
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "SysTypes.h"

//! always compiled for __cplusplus
//! try to compile in several different environments:
//!
//! 1. Windows Static Library = _WIN32 (NOT WIN32, NOT _WINDLL) if GRAY_STATICLIB
//! 2. Windows DLL = _WIN32, _WINDLL, GRAY_DLL (opposite of GRAY_STATICLIB)
//! 3. Windows MFC DLL with Gray* Static Library= _WIN32, _AFXDLL, _MFC_VER > 0x0600
//! 4. WindowsCE / PocketPC= UNDER_CE, _WIN32
//! 5. LINUX 32/64 bit static lib = __linux__ (NOT _BSD?) ( USE_64BIT = __ia64__, __s390x__, __x86_64__, __powerpc64__ )
//! 6. LINUX 32/64 bit SO module = __linux__ 
//! 7. __APPLE__ ??

// M$ compiler versions:
// _MSC_VER=1000 = VS5 = can do "#pragma once",
// _MSC_VER=1200 = VS6
// _MSC_VER=1202 = eVC++4
// _MSC_VER=1300 = VC++ 7.0, NET 2002, can do "__super"
// _MSC_VER=1310 = VC++ 7.1, NET 2003
// _MSC_VER=1400 = VC++ 8 = VS 2005,
// _MSC_VER=1500 = VC++ 9 = VS 2008,
// _MSC_VER=1600 = VC++ 10 = VS 2010,
// _MSC_VER=1700 = VC++ 12 = VS 2012, compiler is 17.00.51106.1
// _MSC_VER=1800 = VC++ 13 = VS 2013,
// _MSC_VER=1900 = VS v14 = VS 2015, (PlatformToolset = 140)
// _MSC_VER=1910 = VS v15 = VS 2017, (PlatformToolset = 141)

// what platform/OS target version of windows minimum/assumed?
// Windows 95 			_WIN32_WINDOWS>=0x0400, WINVER>=0x0400
// Windows NT 4.0 		_WIN32_WINNT>=0x0400,	WINVER>=0x0400
// Windows 98 			_WIN32_WINDOWS>=0x0410, WINVER>=0x0410
// Windows Me 			_WIN32_WINDOWS=0x0500,	WINVER>=0x0500
// Windows 2000 		_WIN32_WINNT>=0x0500,	WINVER>=0x0500
// Windows XP 			_WIN32_WINNT>=0x0501,	WINVER>=0x0501
// Windows Server 2003 	_WIN32_WINNT>=0x0502,	WINVER>=0x0502
// Windows Vista 		_WIN32_WINNT>=0x0600,	WINVER>=0x0600  (lowest we support 2017)
// Windows 7 			_WIN32_WINNT>=0x0601,	WINVER>=0x0601
// Windows Server 2012											(NT 6.2)
// Windows 8 			_WIN32_WINNT>=0x0602,	WINVER>=0x0602	(NT 6.2)
// Windows 8.1
// Windows 10 			_WIN32_WINNT>=0x0A00,	WINVER>=0x0A00

// Other external #define(s) observed:
// __linux__ = Build for Linux OS.
// _DEBUG = debug code. generated by compiler 7 on /MTd or /Mdd option. These options specify debug versions of the C run-time library
// _WIN32 = Build for Windows OS. Is defined for both 64 and 32 bit versions. generated by compiler 7 on.
// WIN32 = use _WIN32 instead although some code seems to look at this as well. Deprecated?
// _WIN64 = 64 bit windows. will also set USE_64BIT and _WIN32
// _M_IX86, _M_X64, _M_AMD64 = what processor type assumed? _M_IX86 only for use of _asm
// _MT = multi-threaded vs. single-threaded ? ASSUME __linux__ IS ALWAYS _MT. non _MT may be deprecated?
// __GNUC__ = using the GCC GNU C compiler (uses __asm__) May work for _WIN32 or __linux__
//
// other compilers:
// __MINGW32__, __BORLANDC__, __WATCOMC__

#if defined(_MSC_VER) && (_MSC_VER >= 1000)
// Remove globally annoying warnings.
#pragma warning(disable:4800)	// "forcing value to bool 'true' or 'false' (performance warning)" (convert BOOL to bool)
#pragma warning(disable:4251)	// 'CC' needs to have dll - interface to be used by clients of class 'Gray::HResult' (FOR DLLs ONLY)

// Useless MSVC2010 warnings: at /W4 level
#pragma warning(disable:4510)	// default constructor could not be generated
#pragma warning(disable:4512)	// assignment operator could not be generated
#pragma warning(disable:4610)	// class 'Gray::CTypeInfo' can never be instantiated - user defined constructor required

#endif // _MSC_VER >= 1000

// see CMemT::NtoH() and CMemT::HtoN() for auto conversion to big endian (network)
// e.g. on little endian (Intel). 0x123456 = 56 34 12 00
#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IA64) || defined(_AMD64_) || defined(__ia64__) || defined(__x86_64__)
#define USE_LITTLE_ENDIAN 1		//!< Intel = little endian = high values in high memory. increasing numeric significance with increasing memory addresses
#endif

// Ignore the system #define UNICODE and _UNICODE. I use my own USE_UNICODE system as default char type. We don't have to use UNICODE just because the system does.
#if defined(UNICODE) || defined(_UNICODE)
#define USE_UNICODE				//!< This allows the including of core headers that use conflicting #define UNICODE to still work.
#define USE_UNICODE_FN			//!< make file names UNICODE as well.
#endif

#if defined(_DEBUG) // || defined(GRAY_STATICLIB)
#define USE_UNITTESTS	//!< Compile in the unit test code. Calling it or not is another matter. in static library case the linker will just remove uncalled code.
#endif

namespace Gray		//!< The main namespace for all Core functions.
{
	//! @namespace Gray
	//! The main namespace for all Core functions.
	//! The main/default namespace for Gray library
#ifndef GRAY_NAME
#define GRAY_NAME	Gray		//!< Root name.
#define GRAY_NAMES	"Gray"		//!< Use GRAYNAME for string.
#endif
#define GRAYCALL	__stdcall	//!< so everyone knows the arg passing scheme. don't assume compiler default.

#if ! defined(GRAY_STATICLIB)
#define GRAY_DLL	// We are building Gray DLL/SO instead of static lib. GRAY_STATICLIB
#endif

	// use _LIB && _WINDLL && _MFC_VER to identify the type of LIB build. or it may just be who is including us.
#ifndef GRAYCORE_LINK
#if defined(_MFC_VER) || defined(GRAY_STATICLIB)	// GRAY_STATICLIB or _MFC_VER can be defined to make Gray* all static lib
#define GRAYCORE_LINK
#else
#define GRAYCORE_LINK __DECL_IMPORT	// default is to include from a DLL/SO (GRAY_DLL)
#endif
#endif

#if defined(_DEBUG) || ! defined(_MSC_VER)
#define _LOCCALL        // static (local) calls might have better calling conventions? But turn off during _DEBUG
#else
#define _LOCCALL        __fastcall // Local procedure name modifier. will not stack dump properly!
#endif

#if defined(__GNUC__) || (! defined(_MSC_VER)) || (_MSC_VER < 1600)
#define __noop		((void)0)		// A macro that does nothing. Compiles out some code. do { } while( 0 )
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1600 // No C++11 features.
	// Get rid of C++11 features. e.g. "= delete"
	#define noexcept
	#define override
	#define IS_DELETE
#else
	#define IS_DELETE = delete
#endif

#ifdef __GNUC__
#define IGNORE_WARN_INTERFACE(c)		virtual ~c() {}		// quiet this warning for interfaces. it is real ?
#define IGNORE_WARN_ABSTRACT(c)			IGNORE_WARN_INTERFACE(c)
#else
#define IGNORE_WARN_INTERFACE(c)
#define IGNORE_WARN_ABSTRACT(c)
#endif	// __GNUC__

	typedef UINT_PTR	HASHCODE_t;				//!< could hold a pointer converted to a number? 64 or 32 bit ?
	typedef UINT32		HASHCODE32_t;			//!< always 32 bits.
	const HASHCODE_t	k_HASHCODE_CLEAR = 0;		//!< not a valid index.

#if (_MFC_VER > 0x0600)
	typedef INT_PTR ITERATE_t;		//!< MFC 6 uses INT_PTR for array indexes.
#else
	typedef int ITERATE_t;
#endif
	const ITERATE_t k_ITERATE_BAD = -1;

	typedef size_t COUNT_t;		//!< like size_t but a count of things that might not be bytes. ASSUME unsigned.

#define IS_INDEX_BAD(i,q)		((COUNT_t)(i)>=(COUNT_t)(q))	//!< cast the (likely) int to unsigned to check for negatives.
#define IS_INDEX_GOOD(i,q)		((COUNT_t)(i)<(COUNT_t)(q))		//!< cast the (likely) int to unsigned to check for negatives.

#define IS_INDEX_BAD_ARRAY(i,a)		IS_INDEX_BAD(i,_countof(a))
#define IS_INDEX_GOOD_ARRAY(i,a)	IS_INDEX_GOOD(i,_countof(a))

	template< typename TYPE >
	static inline INT_PTR GET_INDEX_IN(TYPE a, TYPE b)
	{
		//! diff 2 pointers of the same type to get index diff.
		//! Is b an element in array a?
		return(b - a);
	}

	// a structure should be byte packed and not aligned ? use #pragma pack(push,1) as well
#if defined(__MINGW32__) 
#define CATTR_PACKED __attribute__((packed))	// MING compiler uses this to indicate structure packing required.
#else
#define CATTR_PACKED	// _MSC_VER and __GNUC__ use #pragma pack(1) to indicate packing required.
#endif

#define _sizeofm(s,m)	sizeof(((s *)0)->m)	//!< size_t of a structure member (like offsetof()) nullptr
};

#ifdef _MSC_VER
#define CATTR_NORETURN __declspec(noreturn)
#else // __GNUC__
#define CATTR_NORETURN __attribute__((noreturn))
#endif

#ifdef __GNUC__
#define CATTR_CONSTRUCTOR	__attribute__((constructor)) 
#define CATTR_DESTRUCTOR	__attribute__((destructor)) 
#else
#define CATTR_CONSTRUCTOR
#define CATTR_DESTRUCTOR
#endif

// Allow some method to be deprecated. warn the user to change to some new version.
#ifdef __GNUC__
#define CATTR_DEPRECATEDAT(versionNumber, alternative)	__attribute__((deprecated))
#define CATTR_DEPRECATED								__attribute__((deprecated))
#elif _MSC_VER >= 1400
#define CATTR_DEPRECATEDAT(versionNumber, alternative) __declspec(deprecated("[" #versionNumber "] This function is now deprecated. Please use '" #alternative "' instead."))
#define CATTR_DEPRECATED
#else
#define CATTR_DEPRECATEDAT(versionNumber, alternative)
#define CATTR_DEPRECATED
#endif // _WIN32 && MSVS2005

#endif	// _INC_GRAYCORE
