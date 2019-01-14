//
//! @file CSingletonPtr.h
//!	A reference counted singleton
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CSingletonPtr_H
#define _INC_CSingletonPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CSingleton.h"
#include "CSmartPtr.h"

namespace Gray
{
	template <class TYPE>
	class CSingletonSmart : public CSingleton<TYPE>, public CSmartBase
	{
		//! @class Gray::CSingletonSmart
		//! Base class for a CSingleton that is reference counted and lazy loaded.
		//! This will be destroyed when the last reference is released.
		//! e.g. a public service (shared by all) that is loaded on demand and released when no one needs it.
		//! @note These objects are normally CHeapObject, but NOT ALWAYS ! (allow static versions using StaticConstruct() and k_REFCOUNT_STATIC)

	protected:
		CSingletonSmart(TYPE* pObject, const TYPEINFO_t& rAddrCode, int iRefCountStart = 0)
		: CSingleton<TYPE>(pObject, rAddrCode)
		, CSmartBase(iRefCountStart)
		{
			//! typically this == pObject
		}
		CHEAPOBJECT_IMPL;
	};

	template <class TYPE>
	class CSingletonPtr : protected CSmartPtr < TYPE >		// protected for read only.
	{
		//! @class Gray::CSingletonPtr
		//! A reference to a CSingletonSmart<> based TYPE or a type that has both CSingleton and CSmartBase.
		typedef CSmartPtr<TYPE> SUPER_t;

	public:
		CSingletonPtr(bool bInit = true) : CSmartPtr<TYPE>(bInit ? TYPE::get_Single() : nullptr)
		{
			//! @arg bInit = Allocate a reference automatically by default. Attach to CSingletonSmart, false = defer init until later.
		}
		void InitPtr()
		{
			//! If i created an empty CSingletonPtr(false) (as part of some class) this is how I populate it on that classes constructor.
			//! Attach to CSingletonSmart
			this->put_Ptr(TYPE::get_Single());
		}
		void ReleasePtr()
		{
			SUPER_t::ReleasePtr();
		}
		bool isValidPtr() const
		{
			return SUPER_t::isValidPtr();
		}
		TYPE* operator -> () const
		{
			ASSERT_N(this->m_p != nullptr);
			return this->m_p;
		}
#if 1
		TYPE* get_Ptr() const
		{
			//! For use with SMARTS_CAST(x) and SMART_CAST(x) or just use TYPE::get_Single() ? 
			ASSERT_N(this->m_p != nullptr);
			return this->m_p;
		}
#endif
	};
};
#endif
