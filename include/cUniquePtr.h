//
//! @file cUniquePtr.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cUniquePtr_H
#define _INC_cUniquePtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"

namespace Gray
{
	template<class TYPE>
	class cUniquePtr : public cPtrFacade < TYPE >
	{
		//! @class Gray::cUniquePtr
		//! These are sort of dumb "smart pointers" but assume a single reference.
		//! A single reference to a dynamically allocated (heap) class not based on cRefBase. Free on destruct.
		//! Works like OLD STL "auto_ptr<TYPE>" or boost::unique_ptr<TYPE>, std::unique_ptr<TYPE>

		typedef cUniquePtr<TYPE> THIS_t;
		typedef cPtrFacade<TYPE> SUPER_t;

	public:
		cUniquePtr() noexcept
		{
		}
		explicit cUniquePtr(TYPE* pObj) noexcept
			: cPtrFacade<TYPE>(pObj)
		{
			// explicit to make sure we don't copy an allocated pointer accidentally.
		}

	private:
		// Don't allow this ! copy would be risky.
		cUniquePtr(const THIS_t& rObj) noexcept
			: cPtrFacade<TYPE>(nullptr)
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
			//! @note DANGER! Hidden action.
		}

	public:
		~cUniquePtr()
		{
			FreeLast();
		}

		void AllocArray(size_t nSize = 1) noexcept
		{
			FreeLast();
			this->m_p = new TYPE[nSize];
		}
		void AllocArray(size_t nSize, const TYPE* p) noexcept
		{
			AllocArray(nSize);
			if (p != nullptr && this->m_p != nullptr)
			{
				cMem::Copy(this->m_p, p, sizeof(TYPE)*nSize);
			}
		}
		void ReleasePtr() noexcept
		{
			FreeLast();
		}

		// Override operators

		THIS_t& operator = (TYPE* p2) noexcept
		{
			AsignPtr(p2);
			return *this;
		}
		THIS_t& operator = (THIS_t& ref) noexcept
		{
			AsignRef(ref);
			return *this;
		}

		// Accessor ops.
		TYPE& operator * () const
		{
			ASSERT(this->isValidPtr()); return *this->m_p;
		}

		TYPE* operator -> () const
		{
			ASSERT(this->isValidPtr()); return this->m_p;
		}

		// Comparisons.
		bool operator != (const TYPE* p2) const noexcept
		{
			return p2 != this->m_p;
		}
#if _MSC_VER < 1300	// VC 7.0 has trouble converting to const
		bool operator == (const TYPE* p2) const noexcept
		{
			return(p2 == this->m_p);
		}
#endif
 
		void AsignPtr(TYPE* p2) noexcept
		{
			// AKA put_Ptr() ??
			if (p2 != this->m_p)
			{
				FreeLast();
				this->m_p = p2;
			}
		}
		void AsignRef(THIS_t& ref) noexcept
		{
			if (&ref != this)
			{
				FreeLast();
				this->m_p = ref.get_Ptr();
				ref.m_p = nullptr;	// transferred.
			}
		}

	private:
		void FreeLast() noexcept
		{
			if (this->m_p != nullptr)
			{
				TYPE* p2 = this->m_p;
				this->m_p = nullptr;		// clear this in case the destructor refs itself in some odd way.
				delete p2;	// assume no throw. noexcept
			}
		}
	};

	template<class TYPE>
	class cUniquePtr2 : public cUniquePtr < TYPE >
	{
		//! @class Gray::cUniquePtr2
		//! cUniquePtr Allow a copy constructor that does deep copy.
		typedef cUniquePtr2<TYPE> THIS_t;
		typedef cUniquePtr<TYPE> SUPER_t;

	public:
		cUniquePtr2()
		{
		}
		cUniquePtr2(const THIS_t& rObj)
			: cUniquePtr<TYPE>(Dupe(rObj))
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
		}
		cUniquePtr2(const SUPER_t& rObj)
			: cUniquePtr<TYPE>(Dupe(rObj))
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
			//! @note DANGER! Hidden action.
		}
		explicit cUniquePtr2(TYPE* pObj)
			: cUniquePtr<TYPE>(pObj)
		{
			// explicit to make sure we don't copy an allocated pointer accidentally.
		}

		static TYPE* Dupe(const SUPER_t& rObj)
		{
			TYPE* p2 = rObj.get_Ptr();
			if (p2 == nullptr)
				return nullptr;
			return new TYPE(*p2);
		}

		// Override operators
		THIS_t& operator = (TYPE* p2)
		{
			this->AsignPtr(p2);
			return *this;
		}
		THIS_t& operator = (const THIS_t& ref)
		{
			this->AsignPtr(this->Dupe(ref));
			return *this;
		}
	};
}
#endif // _INC_cUniquePtr_H
