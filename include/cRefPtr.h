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

#include "IUnknown.h"
#include "cBits.h"
#include "cInterlockedVal.h"
#include "cMem.h"
#include "cPtrFacade.h"
#include "cPtrTrace.h"

namespace Gray {
#if defined(_DEBUG) && !defined(UNDER_CE)
#define USE_PTRTRACE_REF
#endif
typedef UINT32 REFCOUNT_t;

/// <summary>
/// base class for some derived object that is to be reference counted via cRefPtr.
/// Use with a "Smart Pointer"
/// cRefPtr is similar to std::shared_ptr &lt;TYPE&gt; except the object must be based on cRefBase
/// @note These objects are normally cHeapObject, but NOT ALWAYS ! (allow static versions using StaticConstruct() and k_REFCOUNT_STATIC)
/// @note These objects emulate the COM IUnknown. we may use cIUnkPtr for this also. QueryInterface support is optional.
/// Use IUNKNOWN_DISAMBIG(cRefBase) with this
/// </summary>
class GRAYCORE_LINK cRefBase : public IUnknown {               // virtual
    static const REFCOUNT_t k_REFCOUNT_STATIC = 0x20000000;    /// for structures that are 'static' or stack based. never use delete
    static const REFCOUNT_t k_REFCOUNT_DEBUG = 0x40000000;     /// mark this as debug. (even in release mode)
    static const REFCOUNT_t k_REFCOUNT_DESTRUCT = 0x80000000;  /// we are in the process of destruction.
    static const REFCOUNT_t k_REFCOUNT_MASK = 0xC0000000;      /// hide extra information in the m_nRefCount

    mutable cInterlockedUInt32 m_nRefCount;  /// count the number of refs. Multi-Thread safe. check _MT here ?? Negative NEVER allowed!

 private:
    void _InternalAddRef() noexcept {
#ifdef _DEBUG
        DEBUG_CHECK(isValidObj());
        DEBUG_CHECK(!isDestructing());
        const REFCOUNT_t nRefCount = get_RefCount();
        DEBUG_CHECK(nRefCount < k_REFCOUNT_DEBUG);
#ifdef USE_PTRTRACE_REF
        if (isSmartDebug()) {
            DEBUG_CHECK(nRefCount != 123123);  // dummy for breakpoint.
        }
#endif
#endif
        m_nRefCount.IncV();
    }
    void _InternalRelease() noexcept {
#ifdef _DEBUG
        DEBUG_CHECK(isValidObj());
        DEBUG_CHECK(!isDestructing());
#if 0  // def USE_PTRTRACE_REF
        const REFCOUNT_t nRefCount2 = get_RefCount();
        if (isSmartDebug()) {
            DEBUG_CHECK(nRefCount2 != 123123);  // dummy for breakpoint.
        }
#endif
#endif
        const REFCOUNT_t nRefCount = m_nRefCount.Dec();
        if (nRefCount == 0) {
            onZeroRefCount();  // free my memory. delete this.
        } else {
            DEBUG_CHECK(nRefCount > 0 && nRefCount < k_REFCOUNT_DEBUG);
        }
    }

 protected:
    /// <summary>
    /// I have no references and i'm being destroyed.
    /// Force vtable to include virtual destructor.
    /// ASSUME StaticDestruct() was called if needed.
    /// </summary>
    virtual ~cRefBase() noexcept {
        DEBUG_CHECK(get_RefCount() == 0);
    }

 public:
    explicit cRefBase(REFCOUNT_t nRefCount = 0) noexcept : m_nRefCount(nRefCount) {}

    REFCOUNT_t get_RefCount() const noexcept {
        return m_nRefCount.get_Value() & ~k_REFCOUNT_MASK;
    }

    /// <summary>
    /// get a unique (only on this machine/process instance) hash code.
    /// </summary>
    HASHCODE_t get_HashCode() const noexcept {
        return PtrCastToNum(this);
    }

    /// <summary>
    /// virtualized version of get_HashCode().
    /// </summary>
    /// <returns></returns>
    STDMETHOD_(HASHCODE_t, get_HashCodeX)() const noexcept {
        return get_HashCode();
    }

    /// <summary>
    /// Zero references to this exist so we can destroy it.
    /// NEVER throw in destruct!
    /// do something when no-one wants this anymore. cache or delete?
    /// @note Obviously this should NEVER be called for a static or stack based object.
    ///  use StaticConstruct() for these.
    /// MFC CCmdTarget has similar onZeroRefCount()
    /// </summary>
    /// <returns></returns>
    virtual void onZeroRefCount() noexcept {
        SetDestructing();
        delete this;  // will call my destructor.
    }

    /// <summary>
    /// Is this really a valid object?
    /// IsValidCast = does it have proper vtable ?
    /// </summary>
    /// <returns>bool valid</returns>
    bool isValidObj() const noexcept {
        if (!cMem::IsValidPtr(this)) return false;
#if defined(_DEBUG) && !defined(__GNUC__)
        return IsValidCast<const cRefBase>(this);
#else
        return true;
#endif
    }

    /// COM IUnknown compliant methods.

    /// <summary>
    /// like COM IUnknown::AddRef.
    /// </summary>
    /// <returns>count after this increment.</returns>
    STDMETHOD_(ULONG, AddRef)() override {
        _InternalAddRef();
        return CastN(ULONG, get_RefCount());
    }
    /// <summary>
    /// Release Ref. like COM IUnknown::Release.
    /// </summary>
    /// <returns>count after this decrement.</returns>
    STDMETHOD_(ULONG, Release)() override {
        const REFCOUNT_t nRefCount = get_RefCount();
        _InternalRelease();  // this could get deleted here!
        return CastN(ULONG, nRefCount - 1);
    }

    STDMETHOD(QueryInterface)(const IID& riid, /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject) override {
        //! like COM IUnknown::QueryInterface
        if (cMem::IsEqual(&riid, &__uuidof(IUnknown), sizeof(riid))) {
            *ppvObject = this;
            _InternalAddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;  // E_NOTIMPL
    }
    virtual IUnknown* get_IUnknown() noexcept {  // disambiguate IUnknown base. in the case of confusing multiple inheritance. This really should not be necessary !?
        return this;
    }

#if 0  // def _DEBUG // for testing.
	void IncRefCount() noexcept { // always go through the COM interface!
		AddRef();
	}
	void DecRefCount() noexcept {
		Release();
	}
#else
    inline void IncRefCount() noexcept {
        _InternalAddRef();
    }
    inline void DecRefCount() noexcept {
        _InternalRelease();
    }
#endif

    bool isStaticConstruct() const noexcept {
        //! Was StaticConstruct() called for this ?
        return cBits::HasMask(m_nRefCount.get_Value(), k_REFCOUNT_STATIC);
    }
    /// <summary>
    /// If this is really static, not dynamic. Call this in parents constructor or main (if global).
    /// </summary>
    void StaticConstruct() {
        ASSERT(m_nRefCount.get_Value() == 0);  // only call in constructor!
        m_nRefCount.put_Value(k_REFCOUNT_STATIC);
    }
    /// <summary>
    /// static objects can fix themselves this way.
    /// ASSUME StaticConstruct() called for this.
    /// </summary>
    void StaticDestruct() {
        ASSERT(isStaticConstruct());
        m_nRefCount.put_Value(0);
    }

    bool isDestructing() const noexcept {
        return cBits::HasMask(m_nRefCount.get_Value(), k_REFCOUNT_DESTRUCT);
    }
    /// <summary>
    /// this object is in the act of destruction. Destruct never throw!
    /// </summary>
    void SetDestructing() noexcept {
        if (isDestructing()) return;
        DEBUG_ASSERT(get_RefCount() == 0, "SetDestructing");
        m_nRefCount.put_Value(k_REFCOUNT_DESTRUCT);
    }

    /// <summary>
    /// Is this object marked as debug?
    /// </summary>
    /// <returns></returns>
    bool isSmartDebug() const noexcept {
        return cBits::HasMask(m_nRefCount.get_Value(), k_REFCOUNT_DEBUG);
    }
    /// <summary>
    /// Mark this object as debug. Trace it. maybe object is in the act of destruction?
    /// </summary>
    bool SetSmartDebug() noexcept {
        if (isSmartDebug())  // already marked?
            return false;
        m_nRefCount.AddX(k_REFCOUNT_DEBUG);
        return true;
    }
};

/// <summary>
/// Template for a type specific reference counted (Smart) pointer based on cRefBase.
/// "Smart pointer" to an object. like "com_ptr_t" _com_ptr_t or cComPtr. https://msdn.microsoft.com/en-us/library/hh279674.aspx
/// Just a ref to the object of some type. TYPE must be based on cRefBase
/// similar to boost::shared_ptr<TYPE> and std::shared_ptr<> but the object MUST be based on cRefBase.
/// </summary>
/// <typeparam name="TYPE">cRefBase</typeparam>
template <class TYPE = cRefBase>
class cRefPtr : public cPtrFacade<TYPE>
#ifdef USE_PTRTRACE_REF
    ,
                public cPtrTrace
#endif
{
    typedef cRefPtr<TYPE> THIS_t;
    typedef cPtrFacade<TYPE> SUPER_t;

    /// <summary>
    /// @note IncRefCount can throw !
    /// </summary>
#ifdef USE_PTRTRACE_REF
    void IncRefFirst(const cDebugSourceLine* src = nullptr) noexcept
#else
    void IncRefFirst() noexcept
#endif
    {
        if (SUPER_t::isValidPtr()) {
            cRefBase* p2 = this->get_Ptr();
            p2->IncRefCount();
#ifdef _DEBUG
            DEBUG_CHECK(!isCorruptPtr());
#endif
#ifdef USE_PTRTRACE_IUNK
            if (p2->isSmartDebug()) TraceAttach(typeid(TYPE), p2, src);
#endif
        }
    }

 public:
    cRefPtr() noexcept {}

    /// <summary>
    /// Add a new ref. Does NOT work with cPtrTrace
    /// @note default = assignment will auto destroy previous and use this constructor.
    /// </summary>
    /// <param name="p2"></param>
    cRefPtr(const TYPE* p2) noexcept : SUPER_t(const_cast<TYPE*>(p2)) {
        IncRefFirst();
    }

    /// <summary>
    /// create my own copy constructor.
    /// </summary>
    cRefPtr(const THIS_t& ref) noexcept : SUPER_t(ref.get_Ptr()) {
        // TODO cPtrTrace keep m_Src info though this might not be accurate?
        IncRefFirst();
    }

#ifdef USE_PTRTRACE_REF
    cRefPtr(const TYPE* p2, const cDebugSourceLine& src) : SUPER_t(const_cast<TYPE*>(p2)) {
        //! for use with REF_PTR(v) macro. like cRefPtr<T> name(REF_PTR(v));
        IncRefFirst(&src);
    }
#endif

    ~cRefPtr() {
        ReleasePtr();
    }

    /// <summary>
    /// Not nullptr?
    /// </summary>
    inline bool isValidPtr() const noexcept {
#ifdef _DEBUG
        DEBUG_CHECK(!isCorruptPtr());
#endif
        return SUPER_t::isValidPtr();
    }

    /// <summary>
    /// is this really pointing to what it is supposed to be pointing to?
    /// Mostly just for _DEBUG usage.
    /// </summary>
    /// <returns>true = nullptr or valid typed pointer.</returns>
    bool isCorruptPtr() const noexcept {
        if (!SUPER_t::isValidPtr()) return false;             // nullptr is not corrupt.
        if (!cMem::IsValidApp(this->get_Ptr())) return true;  // isCorruptPtr
        cRefBase* p = this->get_PtrDyn<cRefBase>();
        if (p == nullptr) return true;
        if (p->get_RefCount() <= 0) return true;

#if 0  // Is TYPE properly defined at this location in the header file ?? Forward ref might not compile here.
			TYPE* p2 = this->get_PtrDyn<TYPE>();
			if (p2 == nullptr)
				return true;
#endif

        return false;
    }
    /// <summary>
    /// dec my ref count and set this to nullptr.
    /// </summary>
    void ReleasePtr() {              // override
        TYPE* p2 = this->get_Ptr();  // make local copy.
        if (p2 == nullptr) return;
#ifdef _DEBUG
        DEBUG_CHECK(!isCorruptPtr());
#endif
        this->ClearPtr();  // make sure possible destructor called in DecRefCount don't reuse this.
#ifdef USE_PTRTRACE_REF
        if (p2->isSmartDebug()) TraceRelease();
#endif
        p2->DecRefCount();  // this might delete this ?
    }

    bool SetSmartDebug(const cDebugSourceLine& src) {
        if (!this->isValidPtr()) return false;
        cRefBase* p2 = this->get_Ptr();
        if (p2->SetSmartDebug()) {
#ifdef USE_PTRTRACE_REF
            // This counts as my first ref!
            TraceAttach(typeid(TYPE), p2, &src);
            return true;
#endif
        }
        return false;
    }

    REFCOUNT_t get_RefCount() const noexcept {
        //! @return cRefBase::get_RefCount
        if (!isValidPtr()) return 0;
        return this->get_Ptr()->get_RefCount();
    }

    // Assignment ops.
    THIS_t& operator=(const TYPE* p2) {
        put_Ptr(const_cast<TYPE*>(p2));
        return *this;
    }

    /// <summary>
    /// Copy assignment operator.
    /// @note we need a ref assignment because we need to prevent new objects from being destroyed before assigned.
    /// </summary>
    /// <param name="ref"></param>
    /// <returns></returns>
    THIS_t& operator=(const THIS_t& ref) {
        put_Ptr(ref.get_Ptr());
        return *this;
    }

 protected:
    /// <summary>
    /// If changed, Release previous ref. Attach the new pointer and add a ref.
    /// </summary>
    /// <param name="p"></param>
    void put_Ptr(TYPE* p) {
        if (!SUPER_t::IsEqual(p)) {
            ReleasePtr();
            this->AttachPtr(p);
            IncRefFirst();
        }
    }
};

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template class GRAYCORE_LINK cRefPtr<cRefBase>;
#endif
}  // namespace Gray
#endif  // _INC_cRefPtr_H
