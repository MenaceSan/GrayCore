//
//! @file cPtrFacade.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cPtrFacade_H
#define _INC_cPtrFacade_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Ptr.h"
#include "cDebugAssert.h"
#include "cTypeInfo.h"
#include "cUnitTestDecl.h"

UNITTEST_PREDEF(cPtrTrace)

namespace Gray
{
	class cLogProcessor;

	class GRAYCORE_LINK cPtrTrace
	{
		//! @class Gray::cPtrTrace
		//! Trace each use of the a pointer in cPtrFacade/cIUnkPtr for debug purposes.
		//! If the lock count fails to go to 0 we know who the leaker was. or if the object is deleted but still has refs we can detect that as well.
		//! Add myself to the cPtrTraceMgr table if the m_p pointer is set.

	public:
		const char* m_pszType;		//!< from __typeof(TYPEINFO_t).name()
		cDebugSourceLine m_Src;		//!< where (in code) was m_p set?

	public:
		cPtrTrace(const TYPEINFO_t& TypeInfo) noexcept
			: m_pszType(TypeInfo.name())
		{
		}
		cPtrTrace(const cPtrTrace& ref) noexcept
			: m_pszType(ref.m_pszType), m_Src(ref.m_Src)
		{
			// copy constructor.
		}

		void TraceOpen(void* p);
		void TraceClose(void* p);

		static void GRAYCALL TraceDump(cLogProcessor& log, ITERATE_t iCountExpected);
		UNITTEST_FRIEND(cPtrTrace);
	};

	template<class TYPE>
	class cPtrFacade
	{
		//! @class Gray::cPtrFacade
		//! a class that acts like (wraps) a pointer to TYPE. Not specific to TYPE=cRefBase.
		//! Base for: cExceptionHolder, cLockerT, cNewPtr, cRefPtr, cIUnkPtr, etc.

		typedef cPtrFacade<TYPE> THIS_t;

	protected:
		TYPE* m_p;	//!< Pointer to some object of TYPE.

	public:
		cPtrFacade(TYPE* p = nullptr) noexcept
			: m_p(p)
		{
		}
		cPtrFacade(THIS_t&& ref) noexcept
		{
			//! move constructor.
			this->m_p = ref.m_p; ref.m_p = nullptr;
		}

		bool isValidPtr() const noexcept
		{
			//! Not nullptr?
			return m_p != nullptr;
		}

		TYPE** get_PPtr()
		{
			//! assume this will be used to set the m_p value.
			ASSERT(m_p == nullptr);
			return &m_p;
		}
		TYPE* get_Ptr() const noexcept
		{
			return m_p;
		}

		void put_Ptr(TYPE* p) noexcept
		{
			//! override this to increment a ref count.
			//! similar to AttachPtr() but can add a ref.
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
			//! sets the pointer WITHOUT adding a ref (if overload applicable). like get_PPtr().
			m_p = p;
		}
		TYPE* DetachPtr() noexcept
		{
			//! Do not decrement the reference count when this is destroyed.
			//! Pass the ref outside the smart pointer system. for use with COM interfaces.
			//! same as _WIN32 ATL CComPtr Detach()
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
		operator TYPE*() const noexcept
		{
			return m_p;
		}

		TYPE& get_Ref() const
		{
			ASSERT(m_p != nullptr); return *m_p;
		}

		TYPE& operator * () const
		{
			return get_Ref();
		}

		TYPE* operator -> () const
		{
			ASSERT(m_p != nullptr); return(m_p);
		}

		//! Comparison ops
		bool operator!() const noexcept
		{
			return(m_p == nullptr);
		}
		bool operator != ( /* const*/ TYPE* p2) const noexcept
		{
			return(p2 != m_p);
		}
		bool operator == ( /* const*/ TYPE* p2) const noexcept
		{
			return(p2 == m_p);
		}
	};

	// Similar to COM QueryInterface() this checks to see if the class is supported.
#define SMART_CAST(_DSTCLASS,p)		DYNPTR_CAST(_DSTCLASS,(p).get_Ptr())
#define SMARTS_CAST(_DSTCLASS,p)	CHECKPTR_CAST(_DSTCLASS,(p).get_Ptr())

}

#endif // _INC_cPtrFacade_H
