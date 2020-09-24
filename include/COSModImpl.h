//
//! @file COSModImpl.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_COSModImpl_H
#define _INC_COSModImpl_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "COSModule.h"
#include "CAtom.h"
#include "CArraySort.h"
#include "CSingleton.h"

#ifdef _MFC_VER
#include "SysRes.h"
#include <afxwin.h>         // MFC extensions AFX_EXTENSION_MODULE. includes <afxdll_.h>
#endif

namespace Gray
{
	class GRAYCORE_LINK COSModImpl
	{
		//! @class Gray::COSModImpl
		//! My implementation of a DLL/SO. _WINDLL
		//! Must be only one of these in a single link space for DLL/SO. So it is not a true CSingleton. Always static allocated. NEVER heap or Stack allocated.
		//! Assume g_Module gets defined for the DLL/SO. On some derived class based on COSModImpl named g_Module.
		//! e.g. COSModImpl g_Module("ModuleName");
		//! @todo Support _WIN32 DLL_THREAD_ATTACH and DLL_THREAD_DETACH ?
		//! Similar to MFC AFX_EXTENSION_MODULE DLLModule or CAtlDllModuleT
		//! This might have a corresponding CXObjModulePtr. CIUnkPtr can be used alternatively.

	public:
		const char* m_pszModuleName;	//!< Just derive this from the file name ?
		HMODULE m_hModule;	//!< My HMODULE assigned to me when loaded. should be same as GetModuleHandleForAddr(&g_Module)

#ifdef _MFC_VER
		AFX_EXTENSION_MODULE m_AFXExt;		// I might be a MFC extension module.
#endif

	private:
		bool OnProcessAttach2();

	public:
		COSModImpl(const char* pszModuleName) noexcept;
		virtual ~COSModImpl();

		bool IsLoaded() const noexcept
		{
			return m_pszModuleName != nullptr && m_hModule != HMODULE_NULL;
		}

		virtual bool OnProcessAttach();
		virtual void OnProcessDetach();		// DLL_PROCESS_DETACH

#ifdef _WIN32
		virtual bool DllMain(HINSTANCE hInstDll, DWORD dwReason);
#elif defined(__linux__)
		void SOConstructor();
		void SODestructor()
		{
			OnProcessDetach();
		}
#endif
	};


#ifdef GRAY_DLL	// _WINDLL
	// ASSUME g_Module is defined for DLL/SO. (and is outside namespace)
	// Declare/expose DllMain()
#ifdef _WIN32	// _WINDLL
#define COSMODULE_IMPL(N)  __DECL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hInstDll, DWORD dwReason, LPVOID) { return N::g_Module.DllMain(hInstDll,dwReason); }
#elif defined(__linux__)
#define COSMODULE_IMPL(N)  CATTR_CONSTRUCTOR void _cdecl SOConstructor() { N::g_Module.SOConstructor(); } \
	CATTR_DESTRUCTOR void _cdecl SODestructor() { g_Module.SODestructor(); }
#endif
#endif //GRAY_DLL

}

#endif
