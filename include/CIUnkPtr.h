//
//! @file cIUnkPtr.h
//! Template for a type specific smart/reference counted pointer to a IUnknown based object.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cIUnkPtr_H
#define _INC_cIUnkPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"
#include "cPtrTrace.h"
#include "IUnknown.h"

#if defined(_DEBUG) && ! defined(UNDER_CE)
// #define USE_PTRTRACE_IUNK
#endif

namespace Gray
{
	class cLogProcessor;

	template<class TYPE = IUnknown>
	class cIUnkPtr
		: public cPtrFacade<TYPE>
#ifdef USE_PTRTRACE_IUNK
		, public cPtrTrace
#endif
	{
		//! @class Gray::cIUnkPtr
		//! Smart/reference counted pointer to an IUnknown based object.
		//! like _WIN32 ATL CComPtr<> or "com_ptr_t"
		//! TYPE must be based on IUnknown

		typedef cIUnkPtr<TYPE> THIS_t;
		typedef cPtrFacade<TYPE> SUPER_t;

#ifdef _DEBUG
	public:
		static void AssertIUnk(TYPE* p2)
		{
			if (p2 == nullptr)
				return;
			ASSERT(static_cast<TYPE*>(p2) != nullptr);		// must be based on TYPE
			ASSERT(static_cast<IUnknown*>(p2) != nullptr);	// must be based on IUnknown
		}
#endif

	protected:
		void IncRefFirst()
		{
			//! Initialize the pointer value and add a single reference.
			//! Compliment ReleasePtr()
			//! @note IncRefCount can throw !
			if (this->m_p != nullptr)
			{
#ifdef _DEBUG
				const int iRefCount = (int)this->m_p->AddRef();
				ASSERT(iRefCount >= 1);
				AssertIUnk(this->m_p);
#else
				this->m_p->AddRef();
#endif
#ifdef USE_PTRTRACE_IUNK
				TraceAttach(this->m_p);	// NOTE: m_Src not set! use IUNK_ATTACH() later
#endif
			}
		}

	public:
		//! Construct and destruction
		cIUnkPtr()
#ifdef USE_PTRTRACE_IUNK
			: cPtrTrace(typeid(TYPE))
#endif
		{
		}
		cIUnkPtr(const TYPE* p2)
			: cPtrFacade<TYPE>(const_cast<TYPE*>(p2))
#ifdef USE_PTRTRACE_IUNK
			, cPtrTrace(typeid(TYPE))
#endif
		{
			//! copy
			IncRefFirst();
		}
		cIUnkPtr(const THIS_t& ref)
			: cPtrFacade<TYPE>(ref.get_Ptr())
#ifdef USE_PTRTRACE_IUNK
			, cPtrTrace(typeid(TYPE), ref.get_Ptr(), ref.m_Src)
#endif
		{
			//! copy. using the assignment auto constructor is not working so use this.
			IncRefFirst();
		}

#ifdef USE_PTRTRACE_IUNK
		cIUnkPtr(const TYPE* p2, const cDebugSourceLine& src)
			: cPtrFacade<TYPE>(const_cast<TYPE*>(p2))
			, cPtrTrace(typeid(TYPE), p2, src)
		{
			//! for use with IUNK_PTR(v) macro. like cIUnkPtr<T> name(IUNK_PTR(v)); NOT using IUNK_ATTACH
			IncRefFirst();
		}
#endif

		~cIUnkPtr()
		{
			ReleasePtr();
		}

		int get_RefCount() const
		{
			//! @return the current reference count. Add and remove a ref to get the count.
			if (this->m_p == nullptr)
				return 0;
			const int iRefCount = (int)this->m_p->AddRef();	// ULONG
			this->m_p->Release();
			return iRefCount - 1;
		}
		TYPE** get_PPtr()
		{
			//! use IUNK_GETPPTR() macro to track this with USE_PTRTRACE_IUNK.
			//! QueryInterface() or similar wants a pointer to a pointer to fill in my interface.
			ReleasePtr();
			ASSERT(!this->isValidPtr());
			return &this->m_p;
		}
		void** get_PPtrV()
		{
			//! get a ** to assign the pointer.
			//! assume the caller has added the first reference for me. Don't call AddRef! 
			//! use IUNK_GETPPTRV() macro to track this with USE_PTRTRACE_IUNK.
			//! QueryInterface() and others don't like the typing.
			ReleasePtr();
			ASSERT(!this->isValidPtr());
			TYPE** ppObj = &this->m_p;
			return reinterpret_cast<void**>(ppObj);
		}
		TYPE* GetInterfacePtr() const noexcept
		{
			//! like _com_ptr_t
			return this->m_p;
		}

		void put_Ptr(TYPE* p2)
		{
			if (p2 != this->m_p)
			{
				ReleasePtr();
				this->m_p = p2;
				IncRefFirst();
			}
		}

		HRESULT SetQI(IUnknown* p2, const IID& riid)
		{
			//! Get the riid interface from p2.
			//! Do proper COM style dynamic_cast for Interface using QueryInterface().
			if (p2 == nullptr)
			{
				ReleasePtr();
				return E_NOINTERFACE;
			}
			// Query for TYPE interface. acts like IUNK_GETPPTRV(pInterface, riid)
			TYPE* pInterface = nullptr;
			HRESULT hRes = p2->QueryInterface(riid, OUT reinterpret_cast<void**>(&pInterface));
			if (FAILED(hRes))
			{
				// pInterface = nullptr;
				ReleasePtr();
				return hRes;
			}

#ifdef _DEBUG
			ASSERT(pInterface != nullptr);
			AssertIUnk(pInterface);
#endif
#ifdef USE_PTRTRACE_IUNK
			TraceAttach(p2);	// NOTE: m_Src not set! use IUNK_ATTACH()
#endif

			// Save the interface without AddRef()ing. ASSUME QueryInterface already did that.
			this->m_p = pInterface;
			return hRes;
		}

#ifdef _MSC_VER
		static const IID& GetIID()
		{
			//! ASSUME we have a IID (GUID) defined for this interface.
			//! @note this seems to only work for _MSC_VER.
			return __uuidof(TYPE);
		}
		HRESULT SetQI(IUnknown* p2)
		{
			//! set the proper pointer for this interface.
			//! Do proper COM style dynamic_cast for Interface using QueryInterface.
			return SetQI(p2, GetIID());
		}
#endif // _MSC_VER

		int ReleasePtr()
		{
			//! Compliment SetFirstIUnk()
			//! @return the new reference count
			if (this->m_p == nullptr)
				return 0;

			TYPE* p2 = this->m_p;	// make local copy.
#ifdef _DEBUG
			AssertIUnk(this->m_p);
#endif
#ifdef USE_PTRTRACE_IUNK
			TraceRelease();
#endif
			this->m_p = nullptr;	// make sure possible destructors called in DecRefCount don't reuse this.
			const int iRefCount = (int)p2->Release();	// this might delete this ?
			return iRefCount;
		}

		//! Assignment ops.
		THIS_t& operator = (const TYPE* p2)
		{
			put_Ptr(const_cast<TYPE*>(p2));
			return *this;
		}
		THIS_t& operator = (const THIS_t& p2)
		{
			//! @note we need a ref assignment because we need to prevent new objects from being destroyed before assigned.
			put_Ptr(p2.get_Ptr());
			return *this;
		}

		//! Accessor ops.
		TYPE& operator * () const
		{
			ASSERT(this->isValidPtr()); 
			return *this->m_p;
		}

		TYPE* operator -> () const
		{
			ASSERT(this->isValidPtr());
			return this->m_p;
		}
	};

	// The lowest (un-type checked) smart/reference counted pointer.
	typedef GRAYCORE_LINK cIUnkPtr<> cIUnkBasePtr;

#ifdef USE_PTRTRACE_IUNK
	template<class TYPE>
	class cIUnkTraceHelper
	{
		//! @class Gray::cIUnkTraceHelper
		//! Use this and the corresponding IUNK_GETPPTR macros to insulate against COM (e.g. DirectX) calls that return interfaces with an implied AddRef().
		//! This represents an "open" handle instance to a cIUnkPtr<IUnknown> as passed into an opaque function that might return a pointer with an AddRef().
		//! a single reference to an IUnknown
		//! auto stack based only.
	private:
		cIUnkPtr<TYPE>& m_rpIObj;	//!< track the open IUnk

	public:
		cIUnkTraceHelper(cIUnkPtr<TYPE>& rpObj, const cDebugSourceLine& src)
			: m_rpIObj(rpObj)
		{
			ASSERT(rpObj.get_Ptr() == nullptr);
			rpObj.m_Src = src;	// last open on this handle.
		}
		~cIUnkTraceHelper()
		{
			//! We allowed something to place a pointer here, so check it.
			TYPE* p = m_rpIObj.get_Ptr();
			if (p != nullptr)
			{
#ifdef _DEBUG
				m_rpIObj.AssertIUnk(p);
#endif
				m_rpIObj.TraceAttach(p);
			}
		}
		operator TYPE** () const
		{
			// the opaque function wants TYPE**
			return m_rpIObj.get_PPtr();
		}
		operator void** () const
		{
			// the opaque function wants void**
			return m_rpIObj.get_PPtrV();
		}
	};

#define IUNK_GETPPTR(p,TYPE)	cIUnkTraceHelper<TYPE>(p,DEBUGSOURCELINE)
#define IUNK_GETPPTRV(p,TYPE)	cIUnkTraceHelper<TYPE>(p,DEBUGSOURCELINE)
#define IUNK_ATTACH(p)			(p).Attach((p).get_Ptr(), DEBUGSOURCELINE);	// attach cPtrTrace to DEBUGSOURCELINE.
#else
#define IUNK_GETPPTR(p,TYPE)	(p).get_PPtr()
#define IUNK_GETPPTRV(p,TYPE)	(p).get_PPtrV()
#define IUNK_ATTACH(p)			__noop		// No trace. do nothing.
#endif	// USE_PTRTRACE_IUNK

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cIUnkPtr<IUnknown>;
#endif

}
#endif // _INC_IUnkPtr_H
