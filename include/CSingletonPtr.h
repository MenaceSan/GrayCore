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
	class cSingletonPtr : protected cRefPtr < TYPE >		// cRefPtr protected for read only.
	{
		//! @class Gray::cSingletonPtr
		//! A reference to a cSingletonRefBase<> based TYPE or a type that has both cSingleton and cRefBase.
		//! A Lazy loaded singleton.
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

		// cRefPtr is protected so expose the parts i allow. cPtrFacade
		void ReleasePtr() 
		{
			SUPER_t::ReleasePtr();
		}
		inline bool isValidPtr() const noexcept
		{
			return SUPER_t::isValidPtr();
		}
		inline TYPE* get_Ptr() const noexcept
		{
			//! expose this otherwise protected function.
			DEBUG_CHECK(this->m_p != nullptr);
			return this->m_p;
		}
		template <class _DST_TYPE>
		_DST_TYPE* get_PtrT() const
		{
			//! Cast pointer to another type.
			//! This is probably a compile time up-cast but check it anyhow.
			//! This shouldn't return nullptr if not starting as nullptr.
			if (this == nullptr)
				return nullptr;
			return CHECKPTR_CAST(_DST_TYPE, get_Ptr());	// dynamic for DEBUG only. Should NEVER return nullptr here !
		}

		inline operator TYPE* () const noexcept
		{
			DEBUG_CHECK(this->m_p != nullptr);
			return this->m_p;
		}
		inline operator TYPE& () const noexcept
		{
			DEBUG_CHECK(this->m_p != nullptr);
			return *this->m_p;
		}
		inline TYPE* operator -> () const noexcept
		{
			DEBUG_CHECK(this->m_p != nullptr);
			return this->m_p;
		}
	};
}
#endif
