//
//! @file cNewPtr.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cNewPtr_H
#define _INC_cNewPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"

namespace Gray
{
	template<class TYPE>
	class cNewPtr : public cPtrFacade < TYPE >
	{
		//! @class Gray::cNewPtr
		//! These are sort of dumb "smart pointers" but assume a single reference.
		//! A single reference to a dynamically allocated (heap) class not based on cRefBase. Free on destruct.
		//! Works like STL "auto_ptr<TYPE>" or boost::unique_ptr<>, std::unique_ptr<>

		typedef cNewPtr<TYPE> THIS_t;
		typedef cPtrFacade<TYPE> SUPER_t;

	public:
		cNewPtr() noexcept
		{
		}
		explicit cNewPtr(TYPE* pObj) noexcept
			: cPtrFacade<TYPE>(pObj)
		{
			// explicit to make sure we don't copy an allocated pointer accidentally.
		}

	private:
		// Don't allow this ! copy would be risky.
		cNewPtr(const THIS_t& rObj) noexcept
			: cPtrFacade<TYPE>(nullptr)
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
		}

	public:
		~cNewPtr()
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
			ASSERT(this->isValidPtr()); return(*this->m_p);
		}

		TYPE* operator -> () const
		{
			ASSERT(this->isValidPtr()); return(this->m_p);
		}

		// Comparisons.
		bool operator != (const TYPE* p2) const noexcept
		{
			return(p2 != this->m_p);
		}
#if _MSC_VER < 1300	// VC 7.0 has trouble converting to const
		bool operator == (const TYPE* p2) const noexcept
		{
			return(p2 == this->m_p);
		}
#endif

	protected:
		void AsignPtr(TYPE* p2) noexcept
		{
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
		void FreeLast() noexcept
		{
			if (this->m_p != nullptr)
			{
				TYPE* p2 = this->m_p;
				this->m_p = nullptr;		// clear this in case the destructor refs itself in some odd way.
				delete p2;
			}
		}
	};

	template<class TYPE>
	class cNewPtr2 : public cNewPtr < TYPE >
	{
		//! @class Gray::cNewPtr2
		//! cNewPtr Allow a copy constructor that does deep copy.
		typedef cNewPtr2<TYPE> THIS_t;
		typedef cNewPtr<TYPE> SUPER_t;

	public:
		cNewPtr2()
		{
		}
		cNewPtr2(const THIS_t& rObj)
			: cNewPtr<TYPE>(Dupe(rObj))
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
		}
		cNewPtr2(const SUPER_t& rObj)
			: cNewPtr<TYPE>(Dupe(rObj))
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
		}
		explicit cNewPtr2(TYPE* pObj)
			: cNewPtr<TYPE>(pObj)
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
#endif // _INC_cNewPtr_H
