//
//! @file cRefPtr.h
//! General object smart pointer mechanism.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cRefPtr_H
#define _INC_cRefPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"
#include "cInterlockedVal.h"
#include "IUnknown.h"
#include "cTimeSys.h"	// TIMESYSD_t
#include "cMem.h"
#include "cPtrTrace.h"

namespace Gray
{

#if defined(_DEBUG) && ! defined(UNDER_CE)
#define USE_PTRTRACE_REF
#endif

	class GRAYCORE_LINK cRefBase : public IUnknown	// virtual
	{
		//! @class Gray::cRefBase
		//! base class for some derived object that is to be reference counted via cRefPtr.
		//! cRefPtr is similar to std::shared_ptr<TYPE> except the object must be based on cRefBase
		//! @note These objects are normally cHeapObject, but NOT ALWAYS ! (allow static versions using StaticConstruct() and k_REFCOUNT_STATIC)
		//! @note These objects emulate the COM IUnknown. we may use cIUnkPtr<> for this also.
		//! Use IUNKNOWN_DISAMBIG(cRefBase) with this

		static const int k_REFCOUNT_DEBUG = 0x20000000;		//!< mark this as debug. (even in release mode)
		static const int k_REFCOUNT_STATIC = 0x40000000;	//!< for structures that are 'static' or stack based. never use delete
		static const int k_REFCOUNT_DESTRUCT = 0x80000000;	//!< we are in the process of destruction.
		static const int k_REFCOUNT_MASK = 0xE0000000;		//!< hide extra information in the m_nRefCount

	private:
		mutable cInterlockedInt m_nRefCount;	//!< count the number of refs. Multi-Thread safe. check _MT here ??

	private:
		void _InternalAddRef() noexcept
		{
#ifdef _DEBUG
			DEBUG_CHECK(isValidObj());
			DEBUG_CHECK(!isDestructing());
			const int iRefCount = get_RefCount();
			if (isSmartDebug())
			{
				DEBUG_CHECK(iRefCount != 123123);	// dummy for breakpoint.
			}
			DEBUG_CHECK(iRefCount < (~k_REFCOUNT_MASK));
#endif
			m_nRefCount.IncV();
		}
		void _InternalRelease() noexcept
		{
#ifdef _DEBUG
			DEBUG_CHECK(isValidObj());
			DEBUG_CHECK(!isDestructing());
			const int iRefCount2 = get_RefCount();
			if (isSmartDebug())
			{
				DEBUG_CHECK(iRefCount2 != 123123); // dummy for breakpoint.
			}
#endif
			const int iRefCount = m_nRefCount.Dec();
			if (iRefCount == 0)
			{
				onFinalRelease();
			}
			else
			{
				DEBUG_CHECK(iRefCount > 0);
			}
		}

	public:

		explicit cRefBase(int iRefCount = 0) noexcept
			: m_nRefCount(iRefCount)
		{
		}
		virtual ~cRefBase() noexcept
		{
			//! ASSUME StaticDestruct() was called if needed.
			DEBUG_CHECK(get_RefCount() == 0);
		}

		int get_RefCount() const noexcept
		{
			return m_nRefCount.get_Value() & ~k_REFCOUNT_MASK;
		}
		HASHCODE_t get_HashCode() const noexcept
		{
			//! get a unique (only on this machine/process instance) hash code.
			return ((HASHCODE_t)(UINT_PTR)(void*)this);
		}
		STDMETHOD_(HASHCODE_t, get_HashCodeX)() const noexcept
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

		bool isValidObj() const noexcept
		{
			// Is this really a valid object?
			// IS_TYPE_OF = does it have proper vtable ?
			if (!cMem::IsValidPtr(this))
				return false;
#if defined(_DEBUG) && ! defined(__GNUC__)
			return IS_TYPE_OF(cRefBase, this);
#else
			return true;
#endif
		}

		// COM IUnknown compliant methods.
		STDMETHOD_(ULONG, AddRef)(void) override
		{
			//! like COM IUnknown::AddRef
			//! @return count after this increment.
			_InternalAddRef();
			return (ULONG)get_RefCount();
		}
		STDMETHOD_(ULONG, Release)(void) override
		{
			//! like COM IUnknown::Release
			//! @return count after this decrement.
			int iRefCount = get_RefCount();
			_InternalRelease();	// this could get deleted here!
			return (ULONG)(iRefCount - 1);
		}
		STDMETHOD(QueryInterface)(const IID& riid, /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject) override
		{
			//! like COM IUnknown::QueryInterface
			if (cMem::IsEqual(&riid, &__uuidof(IUnknown), sizeof(riid)))
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
		inline void IncRefCount() noexcept
		{
			_InternalAddRef();
		}
		inline void DecRefCount()
		{
			_InternalRelease();
		}
#endif

		bool isStaticConstruct() const noexcept
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

		bool isDestructing() noexcept
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

		bool isSmartDebug() const noexcept
		{
			//! Is this object marked as debug?
			return(m_nRefCount.get_Value() & k_REFCOUNT_DEBUG) ? true : false;
		}
		bool SetSmartDebug() noexcept
		{
			//! Mark this object as debug. maybe object is in the act of destruction?
			if (isSmartDebug())	// already marked.
				return false;
			m_nRefCount.AddX(k_REFCOUNT_DEBUG);
			return true;
		}
	};

	template<class TYPE = cRefBase >
	class cRefPtr
		: public cPtrFacade<TYPE>
#ifdef USE_PTRTRACE_REF
		, public cPtrTrace
#endif
	{
		//! @class Gray::cRefPtr
		//! Template for a type specific Smart (reference counted) pointer based on cRefBase.
		//! Smart pointer to an object. like "com_ptr_t" _com_ptr_t or cComPtr. https://msdn.microsoft.com/en-us/library/hh279674.aspx
		//! Just a ref to the object of some type. TYPE must be based on cRefBase
		//! similar to boost::shared_ptr<TYPE> and std::shared_ptr<> but the object MUST be based on cRefBase.

		typedef cRefPtr<TYPE> THIS_t;
		typedef cPtrFacade<TYPE> SUPER_t;

	protected:
		void IncRefFirst() noexcept
		{
			//! @note IncRefCount can throw !
			if (this->m_p != nullptr)
			{
				cRefBase* p = this->m_p;
				p->IncRefCount();
#ifdef USE_PTRTRACE_REF
				if (p->isSmartDebug())
				{
					TraceAttach(p);	// Assume m_Src will get set via call to Attach() later ?? REFPTR_ATTACH
				}
#endif
#ifdef _DEBUG
				DEBUG_CHECK(!isCorruptPtr());
#endif
			}
		}

	public:
		cRefPtr() noexcept
#ifdef USE_PTRTRACE_REF
			: cPtrTrace(typeid(TYPE))
#endif
		{
		}
		cRefPtr(const TYPE* p2) noexcept
			: cPtrFacade<TYPE>(const_cast<TYPE*>(p2))
#ifdef USE_PTRTRACE_REF
			, cPtrTrace(typeid(TYPE))
#endif
		{
			//! add a new ref.
			//! @note default = assignment will auto destroy previous and use this constructor.
			IncRefFirst();
		}
		cRefPtr(const THIS_t& ref) noexcept
			: cPtrFacade<TYPE>(ref.get_Ptr())
#ifdef USE_PTRTRACE_REF
			, cPtrTrace(typeid(TYPE), static_cast<cRefBase*>(ref.get_Ptr()), ref.m_Src)
#endif
		{
			//! create my own copy constructor.
			IncRefFirst();
		}

#ifdef USE_PTRTRACE_REF
		cRefPtr(const TYPE* p2, const cDebugSourceLine& src)
			: cPtrFacade<TYPE>(const_cast<TYPE*>(p2))
			, cPtrTrace(typeid(TYPE), static_cast<cRefBase*>(const_cast<TYPE*>(p2)), src)
		{
			//! for use with REF_PTR(v) macro. like cRefPtr<T> name(REF_PTR(v)); NOT using REFPTR_ATTACH
			IncRefFirst();
		}
		cRefPtr(THIS_t&& ref) noexcept
			: cPtrFacade<TYPE>(ref.get_Ptr())
			, cPtrTrace(typeid(TYPE), static_cast<cRefBase*>(ref.get_Ptr()), ref.m_Src)
		{
			//! move constructor.
			//! move constructor must be done manually in this case. Since we track pointer to the instance.
			if (ref.isValidPtr())
			{
				// cRefBase* p = this->m_p;
				// only if if (p->isSmartDebug()) // ?
				IncRefFirst();
				ref.ReleasePtr();
			}
		}
#endif

		~cRefPtr()
		{
			ReleasePtr();
		}

		inline bool isValidPtr() const noexcept
		{
			//! Not nullptr?
#ifdef _DEBUG
			DEBUG_CHECK(!isCorruptPtr());
#endif
			return this->m_p != nullptr;
		}
		bool isCorruptPtr() const noexcept
		{
			//! is this really pointing to what it is supposed to be pointing to. type check.
			//! Mostly just for _DEBUG usage.
			if (this->m_p == nullptr)	// nullptr is not corrupt.
				return false;
			if (!cMem::IsValidApp(this->m_p))	// isCorruptPtr
				return true;
			cRefBase* p = DYNPTR_CAST(cRefBase, this->m_p);
			if (p == nullptr)
				return true;
			if (p->get_RefCount() <= 0)
				return true;

#if 0	// Is TYPE properly defined at this location in the header file ?? Forward ref might not compile here.
			TYPE* p2 = DYNPTR_CAST(TYPE, p);
			if (p2 == nullptr)
				return true;
#endif

			return false;
		}
		void put_Ptr(TYPE* p)	// override
		{
			//! Attach the pointer and add a ref.
			if (!SUPER_t::IsEqual(p))
			{
				ReleasePtr();
				this->AttachPtr(p);
				IncRefFirst();
			}
		}
		void ReleasePtr()  // override
		{
			//! just set this to nullptr.
			TYPE* p2 = this->m_p;  // make local copy.
			if (p2 != nullptr)
			{
#ifdef _DEBUG
				DEBUG_CHECK(!isCorruptPtr());
#endif
				this->m_p = nullptr;	// make sure possible destructors called in DecRefCount don't reuse this.
#ifdef USE_PTRTRACE_REF
				if (p2->isSmartDebug())
				{
					TraceRelease(static_cast<cRefBase*>(p2));
				}
#endif
				p2->DecRefCount();	// this might delete this ?
			}
		}

		bool SetSmartDebug()	// override
		{
			if (!this->isValidPtr())
				return false;
			if (!this->get_Ptr()->SetSmartDebug())
				return false;
#ifdef USE_PTRTRACE_REF
			TraceAttach(static_cast<cRefBase*>(this->get_Ptr()));
#endif
			return true;
		}

		int get_RefCount() const noexcept
		{
			//! @return cRefBase::get_RefCount
			if (!isValidPtr())
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

#if 1
		template<class _TYPE_2> operator cRefPtr<_TYPE_2>() const
		{
			//! explicit ref type conversion - to remove redundant casts
			//! will work only for properly related types
			return cRefPtr<_TYPE_2>(this->m_p); // this will automatically give an error if classes are unrelated. static_cast<_TYPE_2>
		}
#endif

	};

#ifdef USE_PTRTRACE_REF
#define REFPTR_ATTACH(p)	(p).Attach((p).get_Ptr(), DEBUGSOURCELINE);	// attach trace to DEBUGSOURCELINE.
#else
#define REFPTR_ATTACH(p)	__noop
#endif

	// The lowest (un-type checked) smart pointer.
	typedef GRAYCORE_LINK cRefPtr<> cRefBasePtr;

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cRefPtr<cRefBase>;
#endif

}

#endif // _INC_cRefPtr_H
