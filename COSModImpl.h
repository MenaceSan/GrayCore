//
//! @file COSModImpl.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
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
#include <afxext.h>         // MFC extensions AFX_EXTENSION_MODULE
#endif

namespace Gray
{
	DECLARE_INTERFACE(IOSModuleRelease)
	{
		//! @interface Gray::IOSModuleRelease
		//! when a module is released, subscribe to it so we can do cleanup. Destroy objects that require code in that space.
		virtual ITERATE_t ReleaseModule( HMODULE hModule) = 0;
	};

	class GRAYCORE_LINK COSModImpl
	{
		//! @class Gray::COSModImpl
		//! My implementation of a DLL/SO. _WINDLL
		//! Must be only one of these in a single link space for DLL/SO. But it is not a true CSingleton.
		//! Assume g_Module gets defined for the DLL/SO. On some derived class based on COSModImpl named g_Module.
		//! e.g. COSModImpl g_Module("ModuleName");
		//! @todo Support _WIN32 DLL_THREAD_ATTACH and DLL_THREAD_DETACH ?
		//! Similar to MFC AFX_EXTENSION_MODULE DLLModule or CAtlDllModuleT
		//! This might have a corresponding CXObjModulePtr. CIUnkPtr can be used alternatively.

	public:
		const char* m_pszModuleName;	//!< Just derive this from the file name ?
		HMODULE m_hModule;	//!< My HMODULE assigned to me when loaded. should be same as GetModuleHandleForAddr(&g_Module)

#ifdef _MFC_VER
		AFX_EXTENSION_MODULE m_AFXExt;
#endif

	public:
		COSModImpl(const char* pszModuleName);
		virtual ~COSModImpl();

		virtual bool OnProcessAttach();
		virtual void OnProcessDetach();

#ifdef _WIN32
		bool DllMain(HINSTANCE hInstDll, DWORD dwReason);
#elif defined(__linux__)
		void SOConstructor();
		void SODestructor()
		{
			OnProcessDetach();
		}
#endif
	};

#ifdef GRAY_DLL	// _WINDLL
	// ASSUME g_Module is defined for DLL/SO. Do not declare this inside namespace.	
#ifdef _WIN32	// _WINDLL
#define COSMODULE_IMPL()  __DECL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hInstDll, DWORD dwReason, LPVOID) { return g_Module.DllMain(hInstDll,dwReason); }
#elif defined(__linux__)
#define COSMODULE_IMPL()  CATTR_CONSTRUCTOR void _cdecl SOConstructor() { g_Module.SOConstructor(); } \
	CATTR_DESTRUCTOR void _cdecl SODestructor() { g_Module.SODestructor(); }
#endif

	// GrayLib::CXObjModule or CSmartBase GRAY_NAME ??
#define COSMODULE_RegisterModule_IMPL(_TYPEMOD) extern "C" __DECL_EXPORT HRESULT GRAYCALL CATOM_CAT(Gray,_RegisterModule)(DWORD dwGrayLibVer, _TYPEMOD* pModule) { return g_Module.RegisterModule(dwGrayLibVer, pModule); }
#endif //GRAY_DLL

}

#endif


