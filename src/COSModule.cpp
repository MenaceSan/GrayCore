//
//! @file cOSModule.cpp
//! @note
//!  HINSTANCE and HMODULE are usually/sometimes interchangeable.
//! __linux__ link with 'dl' library
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#include "pch.h"
#include "cOSModule.h"
#include "cSystemHelper.h"
#include "cFilePath.h"
#include "cLogMgr.h"
#include "cAppState.h"
#include "cMime.h"

namespace Gray
{
	cOSModule::cOSModule(HMODULE hModule, UINT32 uFlags)
		: m_hModule(hModule)
		, m_uFlags(uFlags)
	{
		//! @arg bHaveRefCount = Must call FreeModuleLast()
	}
	cOSModule::cOSModule(const FILECHAR_t* pszModuleName, UINT32 uFlags)
		: m_hModule(HMODULE_NULL)
		, m_uFlags(k_Load_Normal)	// we be set later.
	{
		//! Use isValidModule()
		AttachModuleName(pszModuleName, uFlags);
	}
	cOSModule::~cOSModule()
	{
		FreeModuleLast();
	}

	StrLen_t cOSModule::GetModulePath(FILECHAR_t* pszModuleName, StrLen_t nSizeMax) const
	{
		//! Get the file path to the loaded module.
		//! @note there is not absolute rule that it must have one.
#ifdef _WIN32
		// m_hModule = HMODULE_NULL = the current module.
		DWORD uRet = _FNF(::GetModuleFileName)((m_hModule == cAppState::get_HModule()) ? HMODULE_NULL : m_hModule,
			pszModuleName, (DWORD)nSizeMax);
		if (uRet <= 0)
		{
			HRESULT hRes = HResult::GetLastDef(E_FAIL);
			DEBUG_WARN(("cOSModule::GetModulePath ERR='%s'", LOGERR(hRes)));
		}
		return uRet;
#else
		cString sModuleName = this->get_Name();
		return StrT::CopyLen(pszModuleName, sModuleName.get_CPtr(), nSizeMax);
#endif
	}

	cStringF cOSModule::get_Name() const
	{
		//! @return Full path to the module.
#ifdef _WIN32
		FILECHAR_t szModuleName[_MAX_PATH];
		szModuleName[0] = '\0';
		StrLen_t dwLen = GetModulePath(szModuleName, STRMAX(szModuleName));
		if (dwLen <= 0)
		{
			return "";
		}
		return szModuleName;
#else
		if (!m_sModuleName.IsEmpty())
			return m_sModuleName;
		if (m_hModule == cAppState::get_HModule())
		{
			// Get the apps module name.
			return cAppState::get_AppFilePath();
		}
		Dl_info info;
		int iRet = ::dladdr(m_hModule, &info);	// TODO: TEST THIS __linux__. MIGHT NOT WORK !
		if (iRet == 0)
		{
			return "";	// I didn't load this and i don't know its name.
		}
		return info.dli_fname;	// the file name.
#endif
	}

	void cOSModule::FreeModuleLast()
	{
		//! Assume someone else will clear m_hModule
		//! @note
		//!  if this is the last ref to the DLL then it will be unloaded!
		//! @note
		//!  If we free a DLL that has vtable stuff in it,
		//!  any objects based on these vtables are now broken!
		if (!isValidModule())
			return;
		if (m_uFlags & cOSModule::k_Load_NoRefCount)	// Don't free.
			return;
		if (!(m_uFlags & cOSModule::k_Load_Preload))
		{
			DEBUG_MSG(("FreeModuleLast('%s')", LOGSTR(get_Name())));
		}
#ifdef _WIN32
		::FreeLibrary(m_hModule);
#elif defined(__linux__)
		::dlclose(m_hModule);
#endif
	}

	void cOSModule::FreeThisModule()
	{
		//! @note don't call this 'FreeModule' since that can be overloaded by _WIN32 "#define".
		if (!isValidModule())
			return;
		FreeModuleLast();
		DetachModule();
	}

	MIME_TYPE GRAYCALL cOSModule::CheckModuleTypeFile(const FILECHAR_t* pszPathName) // static
	{
		//! Does this file appear to be a module/PE type ?
		//! @return 0 = MIME_UNKNOWN, 3=MIME_EXE, 2=MIME_DLL.
		//! @todo __linux__ must check the MIME type on the actual file.

		static const FILECHAR_t* k_Exts[] = // alternate names.
		{
			_FN(MIME_EXT_dll),	// MIME_DLL
			_FN(MIME_EXT_exe),	// MIME_EXE
			_FN(MIME_EXT_ocx),
			_FN(MIME_EXT_so),	// Linux
			nullptr,
		};
		const FILECHAR_t* pszExt = cFilePath::GetFileNameExt(pszPathName);
		if (pszExt == nullptr)	// not true for __linux__. use MIME type.
			return MIME_UNKNOWN; // no
		ITERATE_t i = StrT::TableFind(pszExt, k_Exts);
		if (i < 0)
		{
			return MIME_UNKNOWN; // no = MIME_UNKNOWN
		}
		return (i == 1) ? MIME_EXE : MIME_DLL;
	}

#ifndef UNDER_CE
	HMODULE GRAYCALL cOSModule::GetModuleHandleForAddr(const void* pAddr) // static
	{
		//! Return the handle for the module this pAddr is in. Do NOT increment ref count.
		//! @arg pAddr = a function pointer for the function we are called from.
		//! This code may be part of a shared object or DLL. track its handle in case it is unloaded dynamically.
		//! @return
		//!  cAppState::get_HModule() = just part of the current EXE
		//!  HMODULE_NULL = error;

		if (pAddr == nullptr)
			return HMODULE_NULL;

#ifdef _WIN32
		HMODULE hModule = HMODULE_NULL;
		if (!_FNF(::GetModuleHandleEx)(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(const FILECHAR_t*)pAddr, &hModule))
			return HMODULE_NULL;
		return hModule;
#else
		Dl_info info;
		int iRet = ::dladdr(pAddr, &info);
		if (iRet == 0)
			return HMODULE_NULL;
		return info.dli_fbase;
#endif
	}
#endif

	FARPROC cOSModule::GetSymbolAddress(const char* pszSymbolName) const
	{
		//! Get a Generic function call address in the module. assume nothing about the functions args.
		//! @note No such thing as a UNICODE proc/function/symbol name! object formats existed before UNICODE.
		//! except for UNDER_CE which does support UNICODE proc names !?
		//! @note this does not work if loaded using LOAD_LIBRARY_AS_IMAGE_RESOURCE or LOAD_LIBRARY_AS_DATAFILE
		ASSERT(isValidModule());
#ifdef UNDER_CE
		return ::GetProcAddressA(m_hModule, pszSymbolName);
#elif defined(_WIN32)
		return ::GetProcAddress(m_hModule, pszSymbolName);
#elif defined(__linux__)
		// add an _ to the front of the name ?
		return (FARPROC) ::dlsym(m_hModule, pszSymbolName);
#endif
	}

	bool cOSModule::AttachModuleName(const FILECHAR_t* pszModuleName, UINT32 uFlags)
	{
		//! is the DLL/SO already loaded? Find it by name.
		//! Full Path is NOT necessary
		//! @arg uFlags = cOSModule::k_Load_NoRefCount

		FreeThisModule();

#ifdef _WIN32
#if ( _WIN32_WINNT > 0x0500 ) && ! defined(UNDER_CE)
		if (!_FNF(::GetModuleHandleEx)(
			(uFlags&k_Load_NoRefCount) ? GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT : 0,
			pszModuleName, &m_hModule))
		{
			ClearModule();
			return false;
		}
		if (!isValidModule())
		{
			return false;
		}
#else
		m_hModule = _FNF(::GetModuleHandle)(pszModuleName);	// no ref count.
		if (!isValidModule())
		{
			return false;
		}
		if (!(uFlags&k_Load_NoRefCount))
		{
			// get a new ref by loading it by its name.
#ifndef UNDER_CE
			::SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
			HMODULE hMod2 = _FNF(::LoadLibrary)(get_Name());
			if (hMod2 == HMODULE_NULL)
			{
				uFlags |= k_Load_NoRefCount;	// I didn't get a ref for some reason. though it is loaded.
			}
			else
			{
				m_hModule = hMod2;
			}
		}
#endif
#elif defined(__linux__)
		m_hModule = ::dlopen(pszModuleName, (uFlags & k_Load_OSMask) | RTLD_NOLOAD);	//  (since glibc 2.2) 
		if (!isValidModule())
			return false;
		if (!(uFlags&k_Load_NoRefCount))
		{
			HMODULE hMod2 = ::dlopen(get_Name(), uFlags & k_Load_OSMask);
			if (hMod2 == HMODULE_NULL)
			{
				uFlags |= k_Load_NoRefCount;	// I didn't get a ref for some reason. though it is loaded.
			}
			else
			{
				m_hModule = hMod2;
			}
		}
#endif
		m_uFlags = uFlags;	// found the handle. Do i need to unload it?
		return true;
	}

	HRESULT cOSModule::LoadModule(const FILECHAR_t* pszModuleName, UINT32 uFlags)
	{
		//! @arg uFlags =
		//!  k_Load_ByName = find if its already loaded first. full path isn't important.
		//!  LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_AS_IMAGE_RESOURCE
		//! @note
		//!  We should use cAppStateModuleLoad with this.

		FreeThisModule();

		if (uFlags & cOSModule::k_Load_ByName)
		{
			// Just look for it already loaded first. ignore full path. just use whatever is same name.
			// Solves problem of having several versions of the same DLL (w/diff paths). get rid of path info!
			bool bRet = AttachModuleName(cFilePath::GetFileName(pszModuleName), uFlags);
			if (bRet)
			{
				return S_FALSE;	// already loaded and made a new ref to it.
			}
			// Not already loaded. Try to load it.
			uFlags &= ~cOSModule::k_Load_ByName;
		}

#ifdef _WIN32
		// _WIN32 will add .dll to the name automatically.
#ifndef UNDER_CE
		::SetErrorMode(SEM_FAILCRITICALERRORS);	// no error dialog.
#endif
		m_hModule = _FNF(::LoadLibraryEx)(pszModuleName, nullptr, uFlags & k_Load_OSMask);   // file name of module
#elif defined(__linux__)
		// Linux requires the file extension.
		m_sModuleName = pszModuleName;
		m_hModule = ::dlopen(pszModuleName, uFlags & k_Load_OSMask);	// RTLD_NOW or RTLD_LAZY
#endif
		if (!isValidModule())
		{
			return GetLastErrorDef();
		}
		m_uFlags = uFlags;	// I'm responsible to unload this when I'm done.
		return S_OK;
	}

	HRESULT cOSModule::LoadModuleWithSymbol(const FILECHAR_t* pszModuleName, const char* pszSymbolName)
	{
		//! Load this module ONLY if it exposes this symbol.
		//! @return HRESULT_WIN32_C(ERROR_CALL_NOT_IMPLEMENTED) = I don't have this symbol

#if 0
		HRESULT hRes = LoadModule(pszModuleName, k_Load_Preload | k_Load_ByName);
		if (FAILED(hRes))
			return hRes;

		FARPROC pAddr = GetSymbolAddress(pszSymbolName);
		if (pAddr == nullptr)
		{
			FreeThisModule();
			return HRESULT_WIN32_C(ERROR_CALL_NOT_IMPLEMENTED);
		}

		if (hRes != S_FALSE)
		{
			// It was k_Load_Preload Loaded ! reload correctly.
			hRes = LoadModule(get_Name(), k_Load_Normal);
		}
#else
		HRESULT hRes = LoadModule(pszModuleName, k_Load_ByName);
		if (FAILED(hRes))
			return hRes;
		FARPROC pAddr = GetSymbolAddress(pszSymbolName);
		if (pAddr == nullptr)
		{
			FreeThisModule();
			return HRESULT_WIN32_C(ERROR_CALL_NOT_IMPLEMENTED);
		}
#endif

		return hRes;
	}
}
 