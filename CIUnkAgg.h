//
//! @file CIUnkAgg.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CIUnkAgg_H
#define _INC_CIUnkAgg_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CSmartPtr.h"

namespace Gray
{
	template<class TYPE = IUnknown>
	class DECLSPEC_NOVTABLE CIUnkAggBase
	{
		//! @class Gray::CIUnkAggBase
		//! Abstract Base for an aggregated interface class. 
		//! This is the base for a class that is a static or member. has no ref count of its own.
		//! This makes interfaces look like multiple inheritance when it really is not. (why not just use multiple inheritance ?)

	protected:
		TYPE* m_pIAggOuter;	//!< the outer object interface.

	public:
		CIUnkAggBase(TYPE* pIAggOuter)
			: m_pIAggOuter(pIAggOuter)
		{
			ASSERT(pIAggOuter != nullptr);
		}
		virtual ~CIUnkAggBase()
		{
		}
		// Support IUnknown functions.
		HRESULT QueryInterface(const IID& riid, void **ppv)
		{
			if (ppv == nullptr || m_pIAggOuter == nullptr)
			{
				*ppv = nullptr;
				return E_POINTER;
			}
			return m_pIAggOuter->QueryInterface(riid, ppv);
		}
		ULONG AddRef(void)
		{
			return m_pIAggOuter->AddRef();
		}
		ULONG Release(void)
		{
			return m_pIAggOuter->Release();
		}
	};

	class DECLSPEC_NOVTABLE CIUnkAgg : public CSmartBase, public CIUnkAggBase < IUnknown >
	{
		//! @class Gray::CIUnkAgg
		//! Abstract Base class for an interface that allows aggregation from some IUnknown parent. (or not)
	public:
		CIUnkAgg(IUnknown* pIAggOuter)
			: CIUnkAggBase((pIAggOuter == nullptr) ? this : pIAggOuter)
		{
		}
		// Support IUknown functions.
		STDMETHODIMP_(ULONG) AddRef(void) override
		{
			if (m_pIAggOuter == this)
			{
				IncRefCount();
				return (ULONG) get_RefCount();
			}
			return m_pIAggOuter->AddRef();
		}
		STDMETHODIMP_(ULONG) Release(void) override
		{
			if (m_pIAggOuter == this)
			{
				DecRefCount();
				return (ULONG) get_RefCount();
			}
			return m_pIAggOuter->Release();
		}
		STDMETHODIMP QueryInterface(const IID& riid, void **ppv) override
		{
			if (ppv == nullptr || m_pIAggOuter == nullptr)
			{
				*ppv = nullptr;
				return E_POINTER;
			}
			if (m_pIAggOuter == this)
			{
				if (riid == __uuidof(IUnknown))
				{
					*ppv = this;
					IncRefCount();
					return S_OK;
				}
				return E_NOINTERFACE;
			}
			return m_pIAggOuter->QueryInterface(riid, ppv);
		}
	};
};
#endif // _INC_CIUnkAgg_H
