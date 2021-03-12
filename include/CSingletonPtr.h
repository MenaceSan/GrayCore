//
//! @file cSingletonPtr.h
//!	A reference counted singleton
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cSingletonPtr_H
#define _INC_cSingletonPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cSingleton.h"
#include "cRefPtr.h"

namespace Gray
{
	template <class TYPE>
	class cSingletonRefBase : public cSingleton<TYPE>, public cRefBase
	{
		//! @class Gray::cSingletonRefBase
		//! Base class for a cSingleton that is reference counted and lazy loaded.
		//! This will be destroyed when the last reference is released. recreated again on demand.
		//! e.g. a public service (shared by all) that is loaded on demand and released when no one needs it.
		//! @note These objects are normally cHeapObject, but NOT ALWAYS ! (allow static versions using StaticConstruct() and k_REFCOUNT_STATIC)

	protected:
		cSingletonRefBase(TYPE* pObject, const TYPEINFO_t& rAddrCode, int iRefCountStart = 0)
			: cSingleton<TYPE>(pObject, rAddrCode)
			, cRefBase(iRefCountStart)
		{
			//! typically this == pObject
		}
		CHEAPOBJECT_IMPL;
	};

	template <class TYPE>
	class cSingletonPtr : protected cRefPtr < TYPE >		// protected for read only.
	{
		//! @class Gray::cSingletonPtr
		//! A reference to a cSingletonRefBase<> based TYPE or a type that has both cSingleton and cRefBase.
		typedef cRefPtr<TYPE> SUPER_t;

	public:
		cSingletonPtr(bool bInit = true) : cRefPtr<TYPE>(bInit ? TYPE::get_Single() : nullptr)
		{
			//! @arg bInit = Allocate a reference automatically by default. Attach to cSingletonRefBase, false = defer init until later.
		}
		void InitPtr()
		{
			//! If i created an empty cSingletonPtr(false) (as part of some class) this is how I populate it on that classes constructor later.
			//! Attach to cSingletonRefBase
			this->put_Ptr(TYPE::get_Single());
		}

		// cRefPtr is protected so expose the parts i allow.
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
			//! expose this otherwise protected function.
			//! For use with SMARTS_CAST(x) and SMART_CAST(x) or just use TYPE::get_Single() ? 
			ASSERT_N(this->m_p != nullptr);
			return this->m_p;
		}
#endif

	};
}
#endif
