//
//! @file cPtrFacade.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPtrFacade_H
#define _INC_cPtrFacade_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "PtrCast.h"
#include "cDebugAssert.h"

namespace Gray
{
	template<class TYPE>
	class cPtrFacade
	{
		//! @class Gray::cPtrFacade
		//! a class that acts like (wraps) a pointer to TYPE. Not specific to TYPE=cRefBase.
		//! Base for: cExceptionHolder, cLockerT, cUniquePtr, cRefPtr, cIUnkPtr, etc.
		//! sizeof(void*)
		//! TODO cPtrNotNull<> // a pointer that can never be nullptr. like gsl::not_null<T>

		typedef cPtrFacade<TYPE> THIS_t;

	protected:
		TYPE* m_p;	//!< Pointer to some object of TYPE.

	public:
		cPtrFacade(TYPE* p = nullptr) noexcept
			: m_p(p)
		{
			// copy
		}
		cPtrFacade(THIS_t&& ref) noexcept
		{
			//! move constructor.
			this->m_p = ref.m_p; ref.m_p = nullptr;
		}

		inline bool isValidPtr() const noexcept
		{
			//! Not nullptr?
			return m_p != nullptr;
		}

		TYPE** get_PPtr()
		{
			//! assume this will be used to set the m_p value.
			ASSERT(m_p == nullptr);		// MUST not have current value.
			return &m_p;
		}
		inline TYPE* get_Ptr() const noexcept
		{
			//! nullptr is allowed.
			return m_p;
		}

		void put_Ptr(TYPE* p) noexcept
		{
			//! override this to increment a ref count.
			//! similar to AttachPtr() but can add a ref.
			//! override this to increment a ref count 
			DEBUG_ASSERT(m_p == nullptr || m_p == p, "put_Ptr");
			m_p = p;
		}
		void ReleasePtr() noexcept
		{
			//! just set this to nullptr.
			//! override this to decrement a ref count or free memory.
			m_p = nullptr;
		}

		void AttachPtr(TYPE* p) noexcept
		{
			//! @note DANGER DONT call this unless you have a good reason. And you know what you are doing !
			//! like put_Ptr() BUT sets the pointer WITHOUT adding a ref (if overload applicable). like get_PPtr(). 
			//! used with Com interfaces where QueryInterface already increments the ref count.
			m_p = p;
		}
		TYPE* DetachPtr() noexcept
		{
			//! Do not decrement the reference count when this is destroyed.
			//! Pass the reference counted pointer outside the smart pointer system. for use with COM interfaces.
			//! same as _WIN32 ATL cComPtr Detach()
			//! @note DANGER DONT call this unless you have a good reason.
			TYPE* p = m_p;
			m_p = nullptr;	// NOT ReleasePtr();
			return p;
		}

		THIS_t& operator = (TYPE* p) noexcept
		{
			//! assignment operator
			m_p = p;
			return *this;
		}
		THIS_t& operator = (THIS_t&& ref) noexcept
		{
			//! move assignment operator
			this->m_p = ref.m_p; ref.m_p = nullptr;
			return *this;
		}

		//! Accessor ops.
		//! @note These are dangerous ! They don't increment the reference count for use !!!
		inline operator TYPE* () const noexcept
		{
			return m_p;
		}

		inline TYPE& get_Ref() const noexcept
		{
			DEBUG_CHECK(m_p != nullptr);
			return *m_p;
		}

		TYPE& operator * () const
		{
			ASSERT(m_p != nullptr);
			return *m_p;;
		}

		TYPE* operator -> () const
		{
			// nullptr is NOT allowed.
			ASSERT(m_p != nullptr);
			return m_p;
		}

		//! Comparison ops
		inline bool operator!() const noexcept
		{
			return m_p == nullptr;
		}
		inline bool operator != ( /* const*/ TYPE* p2) const noexcept
		{
			return p2 != m_p;
		}
		inline bool IsEqual(const TYPE* p2)  const noexcept
		{
			return p2 == m_p;
		}
		inline bool operator == ( /* const*/ TYPE* p2) const noexcept
		{
			return p2 == m_p;
		}

		template <class _DST_TYPE>
		_DST_TYPE* get_PtrT() const
		{
			//! Cast pointer to another type.
			//! This is probably a safe compile time up-cast but check it anyhow.
			//! This shouldn't return nullptr if not starting as nullptr.
			if (m_p == nullptr)	// this is ok.
				return nullptr;
			return CHECKPTR_CAST(_DST_TYPE, m_p);	// dynamic for DEBUG only. Should NEVER return nullptr here !
		}
		template <class _DST_TYPE>
		_DST_TYPE* get_PtrDyn() const
		{
			//! Cast pointer to another type. dynamic_cast
			//! run time dynamic_cast to a (possible) peer type. can return nullptr.
			//! Similar to COM QueryInterface() this checks (dynamically) to see if the class is supported.
			return DYNPTR_CAST(_DST_TYPE, m_p);
		}
	};

	// TODO DELETE  
	// Similar to COM QueryInterface() this checks to see if the class is supported.
#define FACADE_DY_CAST(_DST_TYPE,p) DYNPTR_CAST(_DST_TYPE,(p).get_Ptr())

}

#endif // _INC_cPtrFacade_H
