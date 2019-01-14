//
//! @file CMem.cpp
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#include "StdAfx.h"
#include "COSModImpl.h"
#include "CLogMgr.h"
#include "CDebugAssert.h"
#include "CMem.h"
#include "CAppState.h"

namespace Gray
{
	COSModImpl::COSModImpl(const char* pszModuleName)
		: m_pszModuleName(pszModuleName)
		, m_hModule(HMODULE_NULL)
	{
		ASSERT(! StrT::IsWhitespace(m_pszModuleName));
#ifdef _MFC_VER
		CMem::Zero(&m_AFXExt, sizeof(m_AFXExt));
#endif
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
			ASSERT(m_hModule == HMODULE_NULL);
			ASSERT(hMod != HMODULE_NULL);
			m_hModule = hMod;
#if defined(_DEBUG) && ! defined(UNDER_CE)
			{
				HINSTANCE hInstDllTest = COSModule::GetModuleHandleForAddr(m_pszModuleName);
				ASSERT(m_hModule == hInstDllTest);
			}
#endif
			if (!this->OnProcessAttach())
				return false;
			break;
		case DLL_THREAD_ATTACH:
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
		this->OnProcessAttach();
	}
#endif

}
