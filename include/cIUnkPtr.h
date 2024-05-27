//! @file cIUnkPtr.h
//! Template for a type specific smart/reference counted pointer to a IUnknown based object.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cIUnkPtr_H
#define _INC_cIUnkPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "IUnknown.h"
#include "cPtrFacade.h"
#include "cPtrTrace.h"
#include "cTypeInfo.h"  // typeid()

#if defined(_DEBUG) && !defined(UNDER_CE)
#define USE_PTRTRACE_IUNK
#endif

namespace Gray {
/// <summary>
/// Smart/reference counted pointer to an IUnknown based object.
/// like _WIN32 ATL CComPtr<> or "com_ptr_t"
/// TYPE must be based on IUnknown
/// </summary>
/// <typeparam name="TYPE">IUnknown</typeparam>
template <class TYPE = ::IUnknown>
class cIUnkPtr : public cPtrFacade<TYPE>
#ifdef USE_PTRTRACE_IUNK
    , public cPtrTrace
#endif
{
    typedef cIUnkPtr<TYPE> THIS_t;
    typedef cPtrFacade<TYPE> SUPER_t;

#ifdef _DEBUG
 public:
    static void AssertIUnk(TYPE* p2) {
        if (p2 == nullptr) return;
        ASSERT_NN(static_cast<TYPE*>(p2));        // must be based on TYPE
        ASSERT_NN(static_cast<::IUnknown*>(p2));  // must be based on IUnknown
    }
#endif

 private:
    /// <summary>
    /// Initialize the pointer value and add a single reference.
    /// Compliment ReleasePtr()
    /// @note IncRefCount can throw !
    /// </summary>
#ifdef USE_PTRTRACE_IUNK
    void IncRefFirst(const cDebugSourceLine* src = nullptr)
#else
    void IncRefFirst()
#endif
    {
        if (SUPER_t::isValidPtr()) {
            auto p2 = this->get_Ptr();
#ifdef _DEBUG
            const REFCOUNT_t iRefCount = p2->AddRef();
            ASSERT(iRefCount >= 1);
            AssertIUnk(p2);
#else
            p2->AddRef();
#endif
#ifdef USE_PTRTRACE_IUNK
            TraceAttach(typeid(TYPE), p2, src);
#endif
        }
    }

 public:
    /// Construct and destruction
    cIUnkPtr() {}
    cIUnkPtr(const TYPE* p2) : SUPER_t(const_cast<TYPE*>(p2)) {
        IncRefFirst();
    }
    cIUnkPtr(const THIS_t& ref) : SUPER_t(ref.get_Ptr()) {
        // copy. using the assignment auto constructor is not working so use this.
        // TODO cPtrTrace keep m_Src info though this might not be accurate?
        IncRefFirst();
    }

#ifdef USE_PTRTRACE_IUNK
    /// <summary>
    /// USE_PTRTRACE_IUNK, cPtrTrace specific constructor.
    /// </summary>
    /// <param name="p2"></param>
    /// <param name="src"></param>
    cIUnkPtr(const TYPE* p2, const cDebugSourceLine& src) : SUPER_t(const_cast<TYPE*>(p2)) {
        //! for use with IUNK_PTR(v) macro. like cIUnkPtr<T> name(IUNK_PTR(v)); NOT using IUNK_TRACE
        IncRefFirst(&src);
    }
#endif

    ~cIUnkPtr() {
        ReleasePtr();
    }

    /// <summary>
    /// Get the current reference count. Add and remove a ref to get the count.
    /// </summary>
    REFCOUNT_t get_RefCount() const {
        if (!SUPER_t::isValidPtr()) return 0;
        auto p2 = this->get_Ptr();
        const REFCOUNT_t iRefCount = CastN(REFCOUNT_t, p2->AddRef());  // ULONG
        p2->Release();
        return iRefCount - 1;
    }

    TYPE* GetInterfacePtr() const noexcept {
        //! like _com_ptr_t
        return this->get_Ptr();
    }

    /// <summary>
    /// Set this IUnknown from the riid interface from p2 IUnknown.
    /// Do proper COM style dynamic_cast for Interface using QueryInterface().
    /// </summary>
    /// <param name="p2"></param>
    /// <param name="riid"></param>
    /// <returns></returns>
    HRESULT SetQI(::IUnknown* p2, const IID& riid) {
        ReleasePtr();  // leave it empty.
        if (p2 == nullptr) return E_NOINTERFACE;
        // Query for TYPE interface. acts like IUNK_GETPPTRV(pInterface, riid)
        TYPE* pInterface = nullptr;
        const HRESULT hRes = p2->QueryInterface(riid, OUT reinterpret_cast<void**>(&pInterface));  // get_PPtr()
        if (FAILED(hRes)) return hRes;
#ifdef _DEBUG
        ASSERT(pInterface != nullptr);
        AssertIUnk(pInterface);
#endif
#ifdef USE_PTRTRACE_IUNK
        TraceAttach(typeid(TYPE), p2);  // NOTE: m_Src not set! use IUNK_TRACE()
#endif
        // Save the interface without AddRef()ing. ASSUME QueryInterface already did that.
        this->AttachPtr(pInterface);
        return hRes;
    }

#ifdef _MSC_VER
    static const IID& GetIID() {
        //! ASSUME we have a IID (GUID) defined for this interface.
        //! @note this seems to only work for _MSC_VER.
        return __uuidof(TYPE);
    }
    /// <summary>
    /// set the proper pointer for this interface.
    /// Do proper COM style dynamic_cast for Interface using QueryInterface.
    /// </summary>
    /// <param name="p2"></param>
    /// <returns></returns>
    HRESULT SetQI(::IUnknown* p2) {
        return SetQI(p2, GetIID());
    }
#endif  // _MSC_VER

    /// <summary>
    /// call Release. Compliment SetFirstIUnk().
    /// </summary>
    /// <returns>the new reference count</returns>
    REFCOUNT_t ReleasePtr() {
        if (!SUPER_t::isValidPtr()) return 0;
        TYPE* p2 = this->get_Ptr();  // make local copy.
#ifdef _DEBUG
        AssertIUnk(p2);
#endif
        this->ClearPtr();                                               // make sure possible destructors called in DecRefCount don't reuse this.
        const REFCOUNT_t iRefCount = CastN(REFCOUNT_t, p2->Release());  // this might delete this ?
#ifdef USE_PTRTRACE_IUNK
        TraceRelease();
#endif
        return iRefCount;
    }

    //! Assignment ops.
    THIS_t& operator=(const TYPE* p2) {
        put_Ptr(const_cast<TYPE*>(p2));
        return *this;
    }
    THIS_t& operator=(const THIS_t& p2) {
        //! @note we need a ref assignment because we need to prevent new objects from being destroyed before assigned.
        put_Ptr(p2.get_Ptr());
        return *this;
    }

    // TODO protect THESE !!! ONLY use cIUnkTraceHelper

    TYPE** get_PPtr() {
        //! use IUNK_GETPPTR() macro to track this with USE_PTRTRACE_IUNK. cIUnkTraceHelper
        //! QueryInterface() or similar wants a pointer to a pointer to fill in my interface.
        ReleasePtr();
        ASSERT(!this->isValidPtr());
        return SUPER_t::get_PPtr();
    }
    void** get_PPtrV() {
        //! get a ** to assign the pointer.
        //! assume the caller has added the first reference for me. Don't call AddRef!
        //! use IUNK_GETPPTRV() macro to track this with USE_PTRTRACE_IUNK. cIUnkTraceHelper
        //! QueryInterface() and others don't like the typing.
        ReleasePtr();
        ASSERT(!this->isValidPtr());
        TYPE** ppObj = SUPER_t::get_PPtr();
        return reinterpret_cast<void**>(ppObj);
    }

 protected:
    void put_Ptr(TYPE* p2) {
        if (!SUPER_t::IsEqual(p2)) {
            ReleasePtr();
            this->AttachPtr(p2);
            IncRefFirst();
        }
    }
};

// The lowest (un-type checked) smart/reference counted pointer.
typedef GRAYCORE_LINK cIUnkPtr<> cIUnkBasePtr;

#ifdef USE_PTRTRACE_IUNK
/// <summary>
/// Use this and the corresponding IUNK_GETPPTR macros to insulate against COM (e.g. DirectX) calls that return interfaces with an implied AddRef().
/// This represents an "open" handle instance to a cIUnkPtr<IUnknown> as passed into an opaque function that might return a pointer with an AddRef().
/// a single reference to an IUnknown
/// auto stack based only.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cIUnkTraceHelper {
    cIUnkPtr<TYPE>& m_rpIObj;  /// track the open IUnk
    cDebugSourceLine _Src;

 public:
    cIUnkTraceHelper(cIUnkPtr<TYPE>& rpObj, const cDebugSourceLine& src) : m_rpIObj(rpObj), _Src(src) {
        ASSERT(!rpObj.isValidPtr());
    }
    ~cIUnkTraceHelper() {
        //! We allowed something to place a pointer here, so check it.
        TYPE* p = m_rpIObj.get_Ptr();
        if (p != nullptr) {
#ifdef _DEBUG
            m_rpIObj.AssertIUnk(p);
#endif
            m_rpIObj.TraceAttach(typeid(TYPE), p, &_Src);
        }
    }
    operator TYPE**() const {
        // the opaque function wants TYPE**
        return m_rpIObj.get_PPtr();
    }
    operator void**() const {
        // the opaque function wants void**
        return m_rpIObj.get_PPtrV();
    }
};

#define IUNK_GETPPTR(r, TYPE) cIUnkTraceHelper<TYPE>(r, DEBUGSOURCELINE)
#define IUNK_GETPPTRV(r, TYPE) cIUnkTraceHelper<TYPE>(r, DEBUGSOURCELINE)
#define IUNK_TRACE(p) (p).TraceUpdate(DEBUGSOURCELINE);  // attach cPtrTrace to DEBUGSOURCELINE. late.
#else
#define IUNK_GETPPTR(p, TYPE) (p).get_PPtr()
#define IUNK_GETPPTRV(p, TYPE) (p).get_PPtrV()
#define IUNK_TRACE(p) __noop  // No trace. do nothing.
#endif                        // USE_PTRTRACE_IUNK

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cIUnkPtr<::IUnknown>;
#endif

}  // namespace Gray
#endif  // _INC_IUnkPtr_H
