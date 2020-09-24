//
//! @file COSModule.h
//! Manages links to a *.dll module file.
//! __linux__ link with "dl" library
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_COSModule_H
#define _INC_COSModule_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "COSHandle.h"
#include "CFilePath.h"
#include "CMime.h"

#ifdef _WIN32
#define MODULE_EXT MIME_EXT_dll

#elif defined(__linux__)
#include <dlfcn.h>
#define MODULE_EXT MIME_EXT_so
typedef int (GRAYCALL * FARPROC)();
#else
#error NOOS
#endif

UNITTEST_PREDEF(COSModule)

namespace Gray
{
#define HMODULE_NULL ((HMODULE)nullptr)	// This sometimes means the current process module.
 
	class GRAYCORE_LINK COSModule
	{
		//! @class Gray::COSModule
		//! manage access to a dynamically loaded *.DLL file. (or .SO in __linux__)
		//! in _WIN32 HMODULE is just a load address. Not the same as COSHandle?
		//! ASSUME Default = loaded into my app space ! Use COSModuleX for other processes modules.
		//! Inside a DLL there may be procedures and resources.
		//! DLL's are "shared objects" or "shared libraries" in __linux__
		//! __linux__ link with "dl" library
		//! other times use 1. static binding or 2. delayed binding to DLL
		//! @todo Get module footprint info. how much memory does it use?

	private:
		HMODULE m_hModule;		//!< sometimes the same as HINSTANCE ? = loading address of the code. NOT COSHandle ?
		UINT32 m_uFlags;		//!< k_Load_RefCount= I am responsible to unload this since i loaded it. k_Load_Preload = This is not callable code. init was not called and refs not loaded.
#if defined(__linux__)
		CStringF m_sModuleName;	//!< Must store this if __linux__ can't query it directly.
#endif

	protected:
		void FreeModuleLast();

	public:
#ifdef _WIN32

#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE 
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE      0x00000020		// __GNUC__
#endif
		static const UINT k_Load_Normal = 0;				//!< Flags.
		static const UINT k_Load_Preload = DONT_RESOLVE_DLL_REFERENCES;			//!< 0x01. Just load into memory but don't call any init code. Diff from __linux RTLD_LAZY. No recovery from this.
		static const UINT k_Load_Resource = LOAD_LIBRARY_AS_IMAGE_RESOURCE;		//!< 0x20. This is just a resource load. RTLD_LAZY, LOAD_LIBRARY_AS_IMAGE_RESOURCE
#elif defined(__linux__)
		static const UINT k_Load_Normal = RTLD_NOW;			//!< Flags.
		static const UINT k_Load_Preload = RTLD_LAZY;		//!< Lazy load. Don't call init till we know we need them. Diff from __linux RTLD_LAZY, RTLD_NOLOAD
		static const UINT k_Load_Resource = RTLD_LAZY;		//!< This is just a resource load. RTLD_LAZY, LOAD_LIBRARY_AS_IMAGE_RESOURCE
#endif
		static const UINT k_Load_OSMask		= 0x0FFFFFFF;
		static const UINT k_Load_ByName		= 0x40000000;	//!< try to find it (by just its file name, NOT Path) already loaded first. NOT OS flag.
		static const UINT k_Load_NoRefCount	= 0x80000000;	//!< I DO NOT own the ref count. Don't free. NOT OS Flag.

		COSModule(HMODULE hModule = HMODULE_NULL, UINT32 uFlags = k_Load_Normal);
		COSModule(const FILECHAR_t* pszModuleName, UINT32 uFlags);
		~COSModule();

		static MIME_TYPE GRAYCALL CheckModuleTypeFile(const FILECHAR_t* pszPathName);

#ifndef UNDER_CE
		static HMODULE GRAYCALL GetModuleHandleForAddr(const void* pAddr);
#endif
		FARPROC GetSymbolAddress(const char* pszSymbolName) const;

		bool isValidModule() const noexcept
		{
			return(m_hModule != HMODULE_NULL);
		}
		operator HMODULE() const noexcept
		{
			return m_hModule;
		}
		HMODULE get_ModuleHandle() const noexcept
		{
			return m_hModule;
		}
		UINT_PTR get_ModuleInt() const noexcept
		{
			//! Get the modules handle as an int.
			return (UINT_PTR)m_hModule;
		}
		bool isResourceModule() const noexcept
		{
			// We cant call this. its not loaded as code.
			return ( m_uFlags & (k_Load_Preload | k_Load_Resource));
		}

		StrLen_t GetModulePath(FILECHAR_t* pszModuleName, StrLen_t nSizeMax) const;
		CStringF get_Name() const;

		HRESULT GetLastErrorDef(HRESULT hResDef = E_FAIL) const
		{
#ifdef _WIN32
			return HResult::GetLastDef(hResDef);
#elif defined(__linux__)
			// Just a string?
			const char* pszError = ::dlerror();
			UNREFERENCED_PARAMETER(pszError);
			return E_NOTIMPL;
#endif
		}

		void AttachModule(HMODULE hModule = HMODULE_NULL, UINT32 uFlags = k_Load_Normal)
		{
			FreeModuleLast();
			m_hModule = hModule;
			m_uFlags = uFlags;
		}
		void ClearModule() noexcept
		{
			m_hModule = HMODULE_NULL;
			m_uFlags = k_Load_Normal;
#ifdef __linux__
			m_sModuleName.Empty();
#endif
		}
		HMODULE DetachModule() noexcept
		{
			HMODULE hModule = m_hModule;
			ClearModule();
			return hModule;
		}
		void FreeThisModule();

		bool AttachModuleName(const FILECHAR_t* pszModuleName, UINT32 uFlags = k_Load_NoRefCount);
		HRESULT LoadModule(const FILECHAR_t* pszModuleName, UINT32 uFlags = k_Load_Normal);
		HRESULT LoadModuleWithSymbol(const FILECHAR_t* pszModuleName, const char* pszSymbolName);

#ifdef _WIN32
		// Load or enum resources ?? CWinResource, CAppRes
#endif
		UNITTEST_FRIEND(COSModule);
	};

	template<class TYPE = FARPROC>
	class COSModuleFunc
	{
		//! @class Gray::COSModuleFunc
		//! track a single entry point/procedure/function in the DLL/SO/Module file.
		//! @note It is VERY important to know the proper number and size of args before calling m_pFunc !!! (if not _cdecl)
	public:
		TYPE m_pFunc;		//!< FARPROC to typedef int (FAR WINAPI *FARPROC)();
	public:
		COSModuleFunc(TYPE pFunc = nullptr)
			: m_pFunc(pFunc)
		{
		}
		~COSModuleFunc()
		{
		}
		void ClearFuncAddress()
		{
			m_pFunc = nullptr;
		}
		bool put_FuncAddress(TYPE pFunc)
		{
			m_pFunc = pFunc;
			if (m_pFunc == nullptr)
				return false;
			return true;
		}
		bool put_FuncGeneric(FARPROC pFunc)
		{
			m_pFunc = (TYPE)pFunc;
			if (m_pFunc == nullptr)
				return false;
			return true;
		}
		bool isValidFunc() const
		{
			return(m_pFunc != nullptr);
		}
		operator TYPE () const
		{
			return m_pFunc;
		}
	};

	typedef COSModuleFunc<FARPROC> COSModuleFuncGeneric;

#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK COSModuleFunc < FARPROC >;
#endif
}
#endif // _INC_COSModule_H
