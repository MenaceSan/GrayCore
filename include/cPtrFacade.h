//! @file cPtrFacade.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cPtrFacade_H
#define _INC_cPtrFacade_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "PtrCast.h"

namespace Gray {
/// <summary>
/// a class that acts like (wraps) a pointer to TYPE. Not specific to TYPE=cRefBase.
/// Base for: cExceptionHolder, cLockerT, cUniquePtr, cRefPtr, cIUnkPtr, etc.
/// sizeof(void*)
/// TODO cPtrNotNull // a pointer that can never be nullptr. like gsl::not_null T
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cPtrFacade {
    typedef cPtrFacade<TYPE> THIS_t;

 protected:
    TYPE* m_ptr;  /// Pointer to some object of TYPE.

 protected:
    cPtrFacade(TYPE* p = nullptr) noexcept : m_ptr(p) {}
    cPtrFacade(const THIS_t& ref) : m_ptr(ref.m_ptr) {}
    /// <summary>
    /// move constructor. NOT thread safe.
    /// </summary>
    cPtrFacade(THIS_t&& rref) : m_ptr(rref.m_ptr) {
        rref.m_ptr = nullptr;
    }

    inline TYPE** get_PPtr() noexcept {
        // DANGER
        // DEBUG_CHECK(!isValidPtr());
        return &m_ptr;
    }

    /// <summary>
    /// @note DANGER DONT call this unless you have a good reason. And you know what you are doing !
    /// like put_Ptr() BUT sets the pointer WITHOUT adding a ref (if overload applicable). like get_PPtr().
    /// used with Com interfaces where QueryInterface already increments the ref count.
    /// </summary>
    /// <param name="p"></param>
    void AttachPtr(TYPE* p) noexcept {
        m_ptr = p;
    }
    /// <summary>
    /// just set this to nullptr.
    /// override this to decrement a ref count or free memory.
    /// </summary>
    void ClearPtr() noexcept {
        m_ptr = nullptr;
    }

 public:
    /// <summary>
    /// Not nullptr?
    /// </summary>
    inline bool isValidPtr() const noexcept {
        return m_ptr != nullptr;
    }
    /// <summary>
    /// get pointer. nullptr is allowed.
    /// </summary>
    inline TYPE* get_Ptr() const noexcept {
        return m_ptr;
    }

    /// Accessory ops.
    inline operator TYPE*() const noexcept {
        return m_ptr;
    }
    /// <summary>
    /// Use pointer. nullptr is NOT allowed.
    /// </summary>
    TYPE* operator->() const noexcept {
        DEBUG_CHECK(isValidPtr());
        return m_ptr;
    }

    /// Comparison ops
    inline bool operator!() const noexcept {
        return m_ptr == nullptr;
    }
    inline bool IsEqual(const TYPE* p2) const noexcept {
        return p2 == m_ptr;
    }
    inline bool operator==(/* const*/ TYPE* p2) const noexcept {
        return p2 == m_ptr;
    }
    inline bool operator!=(/* const*/ TYPE* p2) const noexcept {
        return p2 != m_ptr;
    }

    /// <summary>
    /// Cast this pointer to another type.
    /// This is probably a safe compile time up-cast but check it anyhow.
    /// This shouldn't return nullptr if not starting as nullptr.
    /// </summary>
    /// <typeparam name="_DST_TYPE"></typeparam>
    /// <returns></returns>
    template <class _DST_TYPE>
    _DST_TYPE* get_PtrT() const {
        if (m_ptr == nullptr) return nullptr;  // this is ok.
        return PtrCastCheck<_DST_TYPE>(this->get_Ptr());  // dynamic for DEBUG only. Should NEVER return nullptr here !
    }

    /// <summary>
    /// Cast pointer to another type using dynamic_cast
    /// run time dynamic_cast to a (possible) peer type. can return nullptr.
    /// Similar to COM QueryInterface() this checks (dynamically) to see if the class is supported.
    /// </summary>
    /// <typeparam name="_DST_TYPE"></typeparam>
    /// <returns></returns>
    template <class _DST_TYPE>
    _DST_TYPE* get_PtrDyn() const {
        return DYNPTR_CAST(_DST_TYPE, m_ptr);
    }

    /// <summary>
    /// @note DANGER DONT call this unless you have a good reason. And you know what you are doing !
    /// Do not decrement the reference count when this is destroyed.
    /// Pass the reference counted pointer outside the smart pointer system. for use with COM interfaces.
    /// same as _WIN32 ATL cComPtr Detach()
    /// </summary>
    TYPE* DetachPtr() noexcept {
        TYPE* p = m_ptr;
        m_ptr = nullptr;  // NOT ReleasePtr();
        return p;
    }
};

/// Simple array iterator. STL like for use with begin(), end(), for(:)
/// also like BOOST_FOREACH
template <class T>
struct cIterator : cPtrFacade<T> {
    typedef cPtrFacade<T> SUPER_t;
    cIterator(T* p) : SUPER_t(p) {}
    inline void Inc() {
        this->m_ptr++;
    }

    // Prefix increment
    cIterator& operator++() {
        Inc();
        return *this;
    }
    friend bool operator==(const cIterator& a, const cIterator& b) {
        return a.m_ptr == b.m_ptr;
    }
};
}  // namespace Gray
#endif  // _INC_cPtrFacade_H
