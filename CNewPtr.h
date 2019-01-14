//
//! @file CNewPtr.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CNewPtr_H
#define _INC_CNewPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CPtrFacade.h"

namespace Gray
{
	template<class TYPE>
	class CNewPtr : public CPtrFacade < TYPE >
	{
		//! @class Gray::CNewPtr
		//! These are sort of dumb "smart pointers" but assume a single reference.
		//! A single reference to a dynamically allocated (heap) class not based on CSmartBase. Free on destruct.
		//! Works like STL "auto_ptr<TYPE>" or boost::unique_ptr<>, std::unique_ptr<>

		typedef CNewPtr<TYPE> THIS_t;
		typedef CPtrFacade<TYPE> SUPER_t;

	public:
		CNewPtr()
		{
		}
		explicit CNewPtr(TYPE* pObj)
			: CPtrFacade<TYPE>(pObj)
		{
			// explicit to make sure we don't copy an allocated pointer accidentally.
		}

	private:
		// Don't allow this ! copy would be risky.
		CNewPtr(const THIS_t& rObj)
			: CPtrFacade<TYPE>(nullptr)
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
		}

	public:
		~CNewPtr()
		{
			FreeLast();
		}

		void AllocArray(int nSize = 1)
		{
			FreeLast();
			this->m_p = new TYPE[nSize];
		}
		void AllocArray(int nSize, const TYPE* p)
		{
			AllocArray(nSize);
			if (p != nullptr)
			{
				::memcpy(this->m_p, p, sizeof(TYPE)*nSize);
			}
		}
		void ReleasePtr()
		{
			FreeLast();
		}

		// Override operators

		THIS_t& operator = (TYPE* p2)
		{
			AsignPtr(p2);
			return *this;
		}
		THIS_t& operator = (THIS_t& ref)
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
		bool operator != (const TYPE* p2) const
		{
			return(p2 != this->m_p);
		}
#if _MSC_VER < 1300	// VC 7.0 has trouble converting to const
		bool operator == (const TYPE* p2) const
		{
			return(p2 == this->m_p);
		}
#endif

	protected:
		void AsignPtr(TYPE* p2)
		{
			if (p2 != this->m_p)
			{
				FreeLast();
				this->m_p = p2;
			}
		}
		void AsignRef(THIS_t& ref)
		{
			if (&ref != this)
			{
				FreeLast();
				this->m_p = ref.get_Ptr();
				ref.m_p = nullptr;	// transferred.
			}
		}
		void FreeLast()
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
	class CNewPtr2 : public CNewPtr < TYPE >
	{
		//! @class Gray::CNewPtr2
		//! CNewPtr Allow a copy constructor that does deep copy.
		typedef CNewPtr2<TYPE> THIS_t;
		typedef CNewPtr<TYPE> SUPER_t;

	public:
		CNewPtr2()
		{
		}
		CNewPtr2(const THIS_t& rObj)
			: CNewPtr<TYPE>(Dupe(rObj))
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
		}
		CNewPtr2(const SUPER_t& rObj)
			: CNewPtr<TYPE>(Dupe(rObj))
		{
			//! copy the contents? beware performance problems here. I don't know if its a derived type or array?
		}
		explicit CNewPtr2(TYPE* pObj)
			: CNewPtr<TYPE>(pObj)
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

};
#endif // _INC_CNewPtr_H
