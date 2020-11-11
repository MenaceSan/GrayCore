//
//! @file cIUnkPtr.h
//! Template for a type specific smart pointer to a IUnknown based object.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cIUnkPtr_H
#define _INC_cIUnkPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"
#include "IUnknown.h"

#if defined(_DEBUG) && ! defined(UNDER_CE)
#define USE_IUNK_TRACE
#endif

namespace Gray
{
	template<class TYPE = IUnknown>	class cIUnkTraceOpaque;
	class cLogProcessor;

	template<class TYPE = IUnknown>
	class cIUnkPtr
		: public cPtrFacade < TYPE >
#ifdef USE_IUNK_TRACE
		, public cPtrTrace
#endif
	{
		//! @class Gray::cIUnkPtr
		//! Smart pointer to an IUnknown based object.
		//! like _WIN32 ATL CComPtr<> or "com_ptr_t"
		//! TYPE must be based on IUnknown

#ifdef USE_IUNK_TRACE
		friend class cIUnkTraceOpaque < TYPE >;
		friend class cIUnkTraceOpaque < IUnknown >;
#endif
		typedef cIUnkPtr<TYPE> THIS_t;
		typedef cPtrFacade<TYPE> SUPER_t;

	protected:
#ifdef _DEBUG
		static void AssertIUnk(TYPE* p2)
		{
			if (p2 == nullptr)
				return;
			ASSERT(static_cast<TYPE*>(p2) != nullptr);		// must be based on TYPE
			ASSERT(static_cast<IUnknown*>(p2) != nullptr);	// must be based on IUnknown
		}
#endif

		void SetFirstIUnk(TYPE* p2)
		{
			//! Initialize the pointer value and add a single reference.
			//! Compliment ReleasePtr()
			//! @note IncRefCount can throw !
			if (p2 != nullptr)
			{
#ifdef _DEBUG
				int iRefCount = (int)p2->AddRef();
				ASSERT(iRefCount >= 1);
				AssertIUnk(p2);
#else
				p2->AddRef();
#endif
#ifdef USE_IUNK_TRACE
				TraceOpen(p2);	// NOTE: m_Src not set! use IUNK_ATTACH()
#endif
			}
			this->m_p = p2;
		}

	public:
		//! Construct and destruction
		cIUnkPtr()
#ifdef USE_IUNK_TRACE
			: cPtrTrace(typeid(TYPE))
#endif
		{
		}
		cIUnkPtr(const TYPE* p2)
#ifdef USE_IUNK_TRACE
			: cPtrTrace(typeid(TYPE))
#endif
		{
			SetFirstIUnk(const_cast<TYPE*>(p2));
		}
		cIUnkPtr(const THIS_t& ref)
#ifdef USE_IUNK_TRACE
			: cPtrTrace(typeid(TYPE))
#endif
		{
			//! using the assignment auto constructor is not working so use this.
			SetFirstIUnk(ref.get_Ptr());
		}

#if 1
		cIUnkPtr(THIS_t&& ref) // noexcept
#ifdef USE_IUNK_TRACE
			: cPtrTrace(ref)
#endif
		{
			//! move constructor.
			//! would ': cPtrFacade<TYPE>(ref)' deal with cPtrTrace correctly? 			
			this->m_p = ref.m_p; ref.m_p = nullptr;
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
			int iRefCount = (int) this->m_p->AddRef();	// ULONG
			this->m_p->Release();
			return iRefCount - 1;
		}
		TYPE** get_PPtr()
		{
			//! use IUNK_GETPPTR() macro to track this with USE_IUNK_TRACE.
			//! QueryInterface() or similar wants a pointer to a pointer to fill in my interface.
			ReleasePtr();
			ASSERT(!this->isValidPtr());
			return &this->m_p;
		}
		void** get_PPtrV()
		{
			//! use IUNK_GETPPTRV() macro to track this with USE_IUNK_TRACE.
			//! QueryInterface() and others don't like the typing.
			ReleasePtr();
			ASSERT(!this->isValidPtr());
			TYPE** ppObj = &this->m_p;
			return reinterpret_cast<void**>(ppObj);
		}
		TYPE* GetInterfacePtr() const
		{
			//! like _com_ptr_t
			return this->m_p;
		}

		void put_Ptr(TYPE* p2)
		{
			if (p2 != this->m_p)
			{
				ReleasePtr();
				SetFirstIUnk(p2);
			}
		}

		HRESULT SetQI(IUnknown* p2, const IID& riid)
		{
			//! Do proper COM style dynamic_cast for Interface using QueryInterface().
			if (p2 == nullptr)
			{
				ReleasePtr();
				return E_NOINTERFACE;
			}
			// Query for TYPE interface. acts like IUNK_GETPPTRV(pInterface, riid)
			TYPE* pInterface = nullptr;
			HRESULT hRes = p2->QueryInterface(riid, reinterpret_cast<void**>(&pInterface));
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
#ifdef USE_IUNK_TRACE
			TraceOpen(pInterface);	// NOTE: m_Src not set! use IUNK_ATTACH()
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

			TYPE* p2 = this->m_p;
#ifdef _DEBUG
			AssertIUnk(this->m_p);
#endif
#ifdef USE_IUNK_TRACE
			TraceClose(p2);
#endif
			this->m_p = nullptr;	// make sure possible destructors called in DecRefCount don't reuse this.
			int iRefCount = (int)p2->Release();	// this might delete this ?
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
#if 0
		THIS_t& operator = (THIS_t&& ref)
		{
			//! move assignment operator
			this->m_p = ref.m_p; ref.m_p = nullptr;
			return *this;
		}
#endif

		//! Accessor ops.
		TYPE& operator * () const
		{
			ASSERT(this->isValidPtr()); return *this->m_p;
		}

		TYPE* operator -> () const
		{
			ASSERT(this->isValidPtr()); return(this->m_p);
		}
	};

	// The lowest (un-type checked) smart pointer.
	typedef GRAYCORE_LINK cIUnkPtr<> cIUnkBasePtr;

#ifdef USE_IUNK_TRACE
	template<class TYPE>
	class cIUnkTraceOpaque
	{
		//! @class Gray::cIUnkTraceOpaque
		//! This represents an "open" handle instance to a cIUnkPtr<IUnknown> as passed into an opaque function that might return a pointer with an AddRef().
		//! Use this and the corresponding IUNK_GETPPTR macros to insulate against COM (e.g. DirectX) calls that return interfaces with an implied AddRef().
		//! a single reference to an IUnknown
		//! auto stack based only.
	private:
		cIUnkPtr<TYPE>& m_rpIObj;	//!< track the open IUnk
	public:
		cIUnkTraceOpaque(cIUnkPtr<TYPE>& rpObj, const cDebugSourceLine& src)
			: m_rpIObj(rpObj)
		{
			ASSERT(rpObj.get_Ptr() == nullptr);
			rpObj.m_Src = src;	// last open on this handle.
		}
		~cIUnkTraceOpaque()
		{
			//! We allowed something to place a pointer here, so check it.
			TYPE* p = m_rpIObj.get_Ptr();
			if (p != nullptr)
			{
#ifdef _DEBUG
				m_rpIObj.AssertIUnk(p);
#endif
				m_rpIObj.TraceOpen(p);
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

#define IUNK_GETPPTR(p,TYPE)	cIUnkTraceOpaque<TYPE>(p,DEBUGSOURCELINE)
#define IUNK_GETPPTRV(p,TYPE)	cIUnkTraceOpaque<TYPE>(p,DEBUGSOURCELINE)
#define IUNK_ATTACH(p)			ASSERT((p).get_Ptr()!=nullptr); (p).m_Src = DEBUGSOURCELINE; (p).TraceOpen((p).get_Ptr());	// attach trace.
#else
#define IUNK_GETPPTR(p,TYPE)	(p).get_PPtr()
#define IUNK_GETPPTRV(p,TYPE)	(p).get_PPtrV()
#define IUNK_ATTACH(p)			__noop		// No trace.
#endif	// USE_IUNK_TRACE

#ifdef GRAY_DLL // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cIUnkPtr < IUnknown >;
#endif

}
#endif // _INC_IUnkPtr_H
