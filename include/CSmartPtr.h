//
//! @file CSmartPtr.h
//! General object smart pointer mechanism.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CSmartPtr_H
#define _INC_CSmartPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CPtrFacade.h"
#include "CInterlockedVal.h"
#include "IUnknown.h"
#include "CTimeSys.h"	// TIMESYSD_t
#include "CMem.h"

namespace Gray
{
	class GRAYCORE_LINK CSmartBase : public IUnknown	// virtual
	{
		//! @class Gray::CSmartBase
		//! base class for some derived object that is to be reference counted via CSmartPtr.
		//! similar to std::shared_ptr<TYPE> ?
		//! @note These objects are normally CHeapObject, but NOT ALWAYS ! (allow static versions using StaticConstruct() and k_REFCOUNT_STATIC)
		//! @note These objects emulate the COM IUnknown. we may use CIUnkPtr<> for this also.
		//! Use IUNKNOWN_DISAMBIG(CSmartBase) with this

#ifdef _DEBUG
		static const int k_REFCOUNT_DEBUG = 0x20000000;		//!< mark this as debug.
#endif
		static const int k_REFCOUNT_STATIC = 0x40000000;	//!< for structures that are 'static' or stack based. never use delete
		static const int k_REFCOUNT_DESTRUCT = 0x80000000;	//!< we are in the process of destruction.
		static const int k_REFCOUNT_MASK = 0xE0000000;		//!< hide extra information in the m_nRefCount

	private:
		mutable CInterlockedInt m_nRefCount;	//!< count the number of refs. Multi-Thread safe. check _MT here ??

	private:
		void _InternalAddRef()
		{
#ifdef _DEBUG
			ASSERT(isValidObj());
			ASSERT(!isDestructing());
			int iRefCount = get_RefCount();
			if (isSmartDebug())
			{
				ASSERT(iRefCount != 123123);	// dummy for breakpoint.
			}
			ASSERT(iRefCount < (~k_REFCOUNT_MASK));
#endif
			m_nRefCount.IncV();
		}
		void _InternalRelease()
		{
#ifdef _DEBUG
			ASSERT(isValidObj());
			ASSERT(!isDestructing());
			int iRefCount2 = get_RefCount();
			if (isSmartDebug())
			{
				ASSERT(iRefCount2 != 123123); // dummy for breakpoint.
			}
#endif
			int iRefCount = m_nRefCount.Dec();
			if (iRefCount == 0)
			{
				onFinalRelease();
			}
			else
			{
				ASSERT(iRefCount > 0);
			}
		}

	public:

		explicit CSmartBase(int iRefCount = 0) noexcept
			: m_nRefCount(iRefCount)
		{
		}
		virtual ~CSmartBase() 
		{
			//! ASSUME StaticDestruct() was called if needed.
			ASSERT(get_RefCount() == 0);
		}

		int get_RefCount() const noexcept
		{
			return m_nRefCount.get_Value() &~k_REFCOUNT_MASK;
		}
		HASHCODE_t get_HashCode() const noexcept
		{
			//! Unique hash code only on this machine.
			return ((HASHCODE_t)(UINT_PTR)(void*) this);
		}
		STDMETHOD_(HASHCODE_t, get_HashCodeX)() const
		{
			//! virtualized version of get_HashCode.
			return get_HashCode();
		}

		// do something when no-one wants this anymore. cache or delete?
		virtual void onFinalRelease()
		{
			//! Zero references to this exist so we can destroy it.
			//! @note Obviously this should NEVER be called for a static or stack based object.
			//!  use StaticConstruct() for these.
			//! MFC CCmdTarget has similar OnFinalRelease()
			SetDestructing();
			delete this;
		}

		bool isValidObj() const
		{
			if (!CMem::IsValid(this))
				return false;
#if defined(_DEBUG) && ! defined(__GNUC__)
			return DYNPTR_CAST(const CSmartBase, this) != nullptr;
#else
			return true;
#endif
		}

		// COM IUnknown compliant methods.
		STDMETHOD_(ULONG, AddRef)(void) override
		{
			//! like COM IUnknown::AddRef
			_InternalAddRef();
			return (ULONG)get_RefCount();
		}
		STDMETHOD_(ULONG, Release)(void) override
		{
			//! like COM IUnknown::Release
			int iRefCount = get_RefCount();
			_InternalRelease();	// this could get deleted here!
			return (ULONG)(iRefCount - 1);
		}
		STDMETHOD(QueryInterface)(const IID& riid, /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject) override
		{
			//! like COM IUnknown::QueryInterface
			if (CMem::Compare(&riid, &__uuidof(IUnknown), sizeof(riid)) == 0)
			{
				*ppvObject = this;
				_InternalAddRef();
				return S_OK;
			}
			*ppvObject = nullptr;
			return E_NOINTERFACE;	// E_NOTIMPL
		}

#if 0 // def _DEBUG // for testing.
		void IncRefCount()
		{
			AddRef();
		}	// always go through the COM interface!
		void DecRefCount()
		{
			Release();
		}
#else
		void IncRefCount()
		{
			_InternalAddRef();
		}
		void DecRefCount()
		{
			_InternalRelease();
		}
#endif

		bool isStaticConstruct() const
		{
			//! Was StaticConstruct() called for this ?
			return(m_nRefCount.get_Value() & k_REFCOUNT_STATIC) ? true : false;
		}
		void StaticConstruct()
		{
			//! If this is static, not dynamic. Call this in parents constructor or main (if global).
			ASSERT(m_nRefCount.get_Value() == 0);  // only call in constructor!
			m_nRefCount.AddX(k_REFCOUNT_STATIC);
		}
		void StaticDestruct()
		{
			//! static objects can fix themselves this way.
			//! ASSUME StaticConstruct() called for this.
			ASSERT(isStaticConstruct());
			m_nRefCount.put_Value(0);
		}

		bool isDestructing()
		{
			return(m_nRefCount.get_Value() & k_REFCOUNT_DESTRUCT) ? true : false;
		}
		void SetDestructing()
		{
			//! object is in the act of destruction.
			if (isDestructing())
				return;
			ASSERT(get_RefCount() == 0);
			m_nRefCount.put_Value(k_REFCOUNT_DESTRUCT);
		}

#ifdef _DEBUG
		bool isSmartDebug()
		{
			return(m_nRefCount.get_Value() & k_REFCOUNT_DEBUG) ? true : false;
		}
		void SetSmartDebug()
		{
			//! object is in the act of destruction.
			if (isSmartDebug())
				return;
			m_nRefCount.AddX(k_REFCOUNT_DEBUG);
		}
#endif
	};

	template<class TYPE = CSmartBase >
	class CSmartPtr
		: public CPtrFacade < TYPE >
	{
		//! @class Gray::CSmartPtr
		//! Template for a type specific Smart Pointer
		//! Smart pointer to an object. like "com_ptr_t" _com_ptr_t or CComPtr. https://msdn.microsoft.com/en-us/library/hh279674.aspx
		//! Just a ref to the object of some type.
		//! TYPE must be based on CSmartBase
		//! similar to boost::shared_ptr<TYPE>
		//! @todo something like USE_IUNK_TRACE ??

		typedef CSmartPtr<TYPE> THIS_t;
		typedef CPtrFacade<TYPE> SUPER_t;

	protected:
		void IncRefFirst()
		{
			//! @note IncRefCount can throw !
			if (this->m_p != nullptr)
			{
				CSmartBase* p = this->m_p;
				p->IncRefCount();
#ifdef _DEBUG
				ASSERT(!isCorruptPtr());
#endif
			}
		}

	public:
		CSmartPtr()
		{
		}
		CSmartPtr(const TYPE* p2)
			: CPtrFacade<TYPE>(const_cast<TYPE*>(p2))
		{
			//! @note default = assignment will auto destroy previous and use this constructor.
			IncRefFirst();
		}
		CSmartPtr(const THIS_t& ref)
			: CPtrFacade<TYPE>(ref.get_Ptr())
		{
			//! create my own copy constructor.
			IncRefFirst();
		}

#if 0
		CSmartPtr(THIS_t&& ref)
			: CPtrFacade<TYPE>(ref)
		{
			//! move constructor
		}
#endif

		CSmartPtr(const TYPE* p2, TIMESYSD_t dwWaitMS)
			: CPtrFacade<TYPE>(const_cast<TYPE*>(p2))
		{
			//! This is to fake out CThreadLockPtr in single thread mode.
			//! @arg dwWaitMS = ignored.
			UNREFERENCED_PARAMETER(dwWaitMS);
			IncRefFirst();
		}
		~CSmartPtr()
		{
			ReleasePtr();
		}

		bool isValidPtr() const
		{
			//! Not nullptr?
#ifdef _DEBUG
			ASSERT(!isCorruptPtr());
#endif
			return this->m_p != nullptr;
		}
		bool isCorruptPtr() const
		{
			//! is this really pointing to what it is supposed to be pointing to. type check.
			//! Mostly just for _DEBUG usage.
			if (this->m_p == nullptr)	// nullptr is not corrupt.
				return false;
			// ASSERT( DYNPTR_CAST(TYPE,this->m_p) != nullptr );
			CSmartBase* pSmart = DYNPTR_CAST(CSmartBase, this->m_p);
			if (pSmart == nullptr)
				return true;
			if (pSmart->get_RefCount() <= 0)
				return true;
			return false;
		}
		void put_Ptr(TYPE* p)
		{
			//! Attach the pointer and add a ref.
			if (p != this->m_p)
			{
				ReleasePtr();
				this->AttachPtr(p);
				IncRefFirst();
			}
		}
		void ReleasePtr()
		{
			//! just set this to nullptr.
			TYPE* p2 = this->m_p;
			if (p2 != nullptr)
			{
#ifdef _DEBUG
				ASSERT(!isCorruptPtr());
#endif
				this->m_p = nullptr;	// make sure possible destructors called in DecRefCount don't reuse this.
				p2->DecRefCount();	// this might delete this ?
			}
		}
		int get_RefCount() const
		{
			//! @return CSmartBase::get_RefCount
			if (this->m_p == nullptr)
				return 0;
			return this->m_p->get_RefCount();
		}

		// Assignment ops.
		THIS_t& operator = (const TYPE* p2)
		{
			put_Ptr(const_cast<TYPE*>(p2));
			return *this;
		}
		THIS_t& operator = (const THIS_t& ref)
		{
			//! Copy assignment operator.
			//! @note we need a ref assignment because we need to prevent new objects from being destroyed before assigned.
			put_Ptr(ref.get_Ptr());
			return *this;
		}

#if 0
		THIS_t& operator = (THIS_t&& ref)
		{
			//! move assignment operator
			this->m_p = ref.m_p; ref.m_p = nullptr;
			return *this;
		}
#endif

#if 1
		template<class _TYPE_2> operator CSmartPtr<_TYPE_2>() const
		{
			//! explicit ref type conversion - to remove redundant casts
			//! will work only for properly related types
			return CSmartPtr<_TYPE_2>((this->m_p)); // this will automatically give an error if classes are unrelated. static_cast<_TYPE_2>
		}
#endif

	};

	// The lowest (un-type checked) smart pointer.
	typedef GRAYCORE_LINK CSmartPtr<> CSmartBasePtr;

#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK CSmartPtr < CSmartBase >;
#endif

}

#endif // _INC_CSmartPtr_H