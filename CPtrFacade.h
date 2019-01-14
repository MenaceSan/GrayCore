//
//! @file CPtrFacade.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CPtrFacade_H
#define _INC_CPtrFacade_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Ptr.h"
#include "CDebugAssert.h"
#include "CTypeInfo.h"
#include "CUnitTestDecl.h"

UNITTEST_PREDEF(CPtrTrace)

namespace Gray
{
	class CLogProcessor;

	class GRAYCORE_LINK CPtrTrace
	{
		//! @class Gray::CPtrTrace
		//! Trace each use of the a pointer in CPtrFacade/CIUnkPtr for debug purposes.
		//! If the lock count fails to go to 0 we know who the leaker was. or if the object is deleted but still has refs we can detect that as well.
		//! Add myself to the CPtrTraceMgr table if the m_p pointer is set.

	public:
		const char* m_pszType;		//!< from __typeof(TYPEINFO_t).name()
		CDebugSourceLine m_Src;		//!< where (in code) was m_p set?

	public:
		CPtrTrace(const TYPEINFO_t& TypeInfo)
			: m_pszType(TypeInfo.name())
		{
		}
		CPtrTrace(const CPtrTrace& ref)
			: m_pszType(ref.m_pszType), m_Src(ref.m_Src)
		{
			// copy constructor.
		}

		void TraceOpen(void* p);
		void TraceClose(void* p);

		static void GRAYCALL TraceDump(CLogProcessor& log, ITERATE_t iCountExpected);
		UNITTEST_FRIEND(CPtrTrace);
	};

	template<class TYPE>
	class CPtrFacade
	{
		//! @class Gray::CPtrFacade
		//! a class that acts like (wraps) a pointer to TYPE. Not specific to TYPE=CSmartBase.
		//! Base for: cExceptionHolder, CLockerT, CNewPtr, CSmartPtr, CIUnkPtr, etc.

		typedef CPtrFacade<TYPE> THIS_t;

	protected:
		TYPE* m_p;	//!< Pointer to some object of TYPE.

	public:
		CPtrFacade(TYPE* p = nullptr)
			: m_p(p)
		{
		}
		CPtrFacade(THIS_t&& ref) noexcept
		{
			//! move constructor.
			this->m_p = ref.m_p; ref.m_p = nullptr;
		}

		bool isValidPtr() const
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
		TYPE* get_Ptr() const
		{
			return m_p;
		}

		void put_Ptr(TYPE* p)
		{
			//! override this to increment a ref count.
			//! similar to AttachPtr() but can add a ref.
			m_p = p;
		}
		void ReleasePtr()
		{
			//! just set this to nullptr.
			//! override this to decrement a ref count or free memory.
			m_p = nullptr;
		}

		void AttachPtr(TYPE* p)
		{
			//! sets the pointer WITHOUT adding a ref (if overload applicable). like get_PPtr().
			m_p = p;
		}
		TYPE* DetachPtr()
		{
			//! Do not decrement the reference count when this is destroyed.
			//! Pass the ref outside the smart pointer system. for use with COM interfaces.
			//! same as _WIN32 ATL CComPtr Detach()
			TYPE* p = m_p;
			m_p = nullptr;	// NOT ReleasePtr();
			return p;
		}

		THIS_t& operator = (TYPE* p)
		{
			//! assignment operator
			m_p = p;
			return *this;
		}
		THIS_t& operator = (THIS_t&& ref)
		{
			//! move assignment operator
			this->m_p = ref.m_p; ref.m_p = nullptr;
			return *this;
		}

		//! Accessor ops.
		//! @note These are dangerous ! They don't increment the reference count for use !!!
		operator TYPE*() const
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
		bool operator!() const
		{
			return(m_p == nullptr);
		}
		bool operator != ( /* const*/ TYPE* p2) const
		{
			return(p2 != m_p);
		}
		bool operator == ( /* const*/ TYPE* p2) const
		{
			return(p2 == m_p);
		}
	};

	// Similar to COM QueryInterface() this checks to see if the class is supported.
#define SMART_CAST(_DSTCLASS,p)		DYNPTR_CAST(_DSTCLASS,(p).get_Ptr())
#define SMARTS_CAST(_DSTCLASS,p)	CHECKPTR_CAST(_DSTCLASS,(p).get_Ptr())

}

#endif // _INC_CPtrFacade_H
