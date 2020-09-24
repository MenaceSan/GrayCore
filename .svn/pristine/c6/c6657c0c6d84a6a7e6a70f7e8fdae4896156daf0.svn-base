//
//! @file CMem.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "COSModImpl.h"
#include "CLogMgr.h"
#include "CDebugAssert.h"
#include "CMem.h"
#include "CAppState.h"

namespace Gray
{
	COSModImpl::COSModImpl(const char* pszModuleName) noexcept
		: m_pszModuleName(pszModuleName)
		// , m_hModule(HMODULE_NULL)
	{
		// Always static allocated. NEVER heap or Stack allocated. So Zero init is NEVER needed!
		ASSERT(!StrT::IsWhitespace(m_pszModuleName));
		OnProcessAttach2();
	}

	COSModImpl::~COSModImpl() // virtual 
	{
	}

	bool COSModImpl::OnProcessAttach() // virtual 
	{
		// DLL_PROCESS_ATTACH
		DEBUG_MSG(("%s:OnProcessAttach 0%x", LOGSTR(m_pszModuleName), (UINT)(UINT_PTR)m_hModule));

#ifdef _MFC_VER
		// Extension DLL one-time initialization
		if (!::AfxInitExtensionModule(m_AFXExt, (HINSTANCE)m_hModule))
			return false;
		// perhaps new CDynLinkLibrary ? 
#endif
		return true;
	}

	bool COSModImpl::OnProcessAttach2()	// private
	{
		// NOTE: In the LoadModuel (dynamic) case this will get called BEFORE the constructor for COSModDyn.

		if (!IsLoaded())	// Race is over.
			return true;

#if defined(_DEBUG) && ! defined(UNDER_CE)
		HINSTANCE hInstDllTest = COSModule::GetModuleHandleForAddr(m_pszModuleName);
		ASSERT(m_hModule == hInstDllTest);
#endif

		return this->OnProcessAttach();
	}

	void COSModImpl::OnProcessDetach() // virtual 
	{
		// DLL_PROCESS_DETACH
		DEBUG_MSG(("%s:OnProcessDetach 0%x", LOGSTR(m_pszModuleName), (UINT)(UINT_PTR)m_hModule));

#ifdef _MFC_VER
		::AfxTermExtensionModule(m_AFXExt);
#endif

		// Try to release my singletons in proper order.
		CSingletonRegister::ReleaseModule(m_hModule);
	}

#ifdef _WIN32
	bool COSModImpl::DllMain(HINSTANCE hMod, DWORD dwReason)
	{
		switch (dwReason)
		{
		case DLL_PROCESS_DETACH:
			ASSERT(hMod == m_hModule);
			this->OnProcessDetach();
			break;
		case DLL_PROCESS_ATTACH:
			// NOTE: In the LoadModuel (dynamic) case this will get called BEFORE the constructor for COSModDyn.
			ASSERT(m_hModule == HMODULE_NULL);
			ASSERT(hMod != HMODULE_NULL);
			m_hModule = hMod;
			return this->OnProcessAttach2();
		case DLL_THREAD_ATTACH: // a new thread has been created.
		case DLL_THREAD_DETACH:
			break;
		default:
			DEBUG_ERR(("%s:DllMain event=%d", LOGSTR(m_pszModuleName), dwReason));
			break;
		}
		return true;
	}

#elif defined(__linux__)
	void COSModImpl::SOConstructor()
	{
		m_hModule = COSModule::GetModuleHandleForAddr(m_pszModuleName);
		this->OnProcessAttach2();
	}
#endif

}
