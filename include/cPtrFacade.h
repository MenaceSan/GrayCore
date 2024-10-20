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
/// size = sizeof(void*) = _SIZEOF_PTR
/// TODO cPtrNotNull // a pointer that can never be nullptr. like gsl::not_null T
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cPtrFacade {
    typedef cPtrFacade<TYPE> THIS_t;

 protected:
    TYPE* _ptr;  /// Pointer to some object of TYPE.

 protected:
    cPtrFacade(TYPE* p = nullptr) noexcept : _ptr(p) {}
    cPtrFacade(const THIS_t& ref) noexcept : _ptr(ref._ptr) {}
    /// <summary>
    /// move constructor. NOT thread safe.
    /// </summary>
    cPtrFacade(THIS_t&& rref) noexcept : _ptr(rref._ptr) {
        rref._ptr = nullptr;
    }

    /// <summary>
    /// @note DANGER DONT call this unless you have a good reason. And you know what you are doing !
    /// like put_Ptr() BUT sets the pointer WITHOUT adding a ref (if overload applicable). like ref_Ptr().
    /// used with Com interfaces where QueryInterface already increments the ref count.
    /// </summary>
    /// <param name="p"></param>
    void AttachPtr(TYPE* p) noexcept {
        _ptr = p;
    }
    /// <summary>
    /// just set this to nullptr.
    /// override this to decrement a ref count or free memory.
    /// </summary>
    void ClearPtr() noexcept {
        _ptr = nullptr;
    }

    /// <summary>
    /// Get a reference to the pointer.  DANGER I can change the value!
    /// </summary>
    inline TYPE*& ref_Ptr() noexcept {
        // DEBUG_CHECK(!isValidPtr());
        return _ptr;
    }

 public:
    /// <summary>
    /// @note DANGER DONT call this unless you have a good reason. And you know what you are doing !
    /// Do not decrement the reference count when this is destroyed.
    /// Pass the reference counted pointer outside the smart pointer system. for use with COM interfaces.
    /// same as _WIN32 ATL cComPtr Detach()
    /// </summary>
    TYPE* DetachPtr() noexcept {
        TYPE* p = _ptr;
        _ptr = nullptr;  // NOT ReleasePtr();
        return p;
    }

    /// <summary>
    /// Not nullptr?
    /// </summary>
    inline bool isValidPtr() const noexcept {
        return _ptr != nullptr;
    }
    /// <summary>
    /// get pointer. nullptr is allowed.
    /// </summary>
    inline TYPE* get_Ptr() const noexcept {
        return _ptr;
    }

    /// Accessory ops.
    inline operator TYPE*() const noexcept {
        return _ptr;
    }
    /// <summary>
    /// Use pointer. nullptr is NOT allowed.
    /// </summary>
    TYPE* operator->() const noexcept {
        DEBUG_CHECK(isValidPtr());
        return _ptr;
    }

    /// Comparison ops
    inline bool operator!() const noexcept {
        return _ptr == nullptr;
    }
    inline bool IsEqual(const TYPE* p2) const noexcept {
        return p2 == _ptr;
    }
    inline bool operator==(/* const*/ TYPE* p2) const noexcept {
        return p2 == _ptr;
    }
    inline bool operator!=(/* const*/ TYPE* p2) const noexcept {
        return p2 != _ptr;
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
        if (_ptr == nullptr) return nullptr;             // this is ok.
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
        return DYNPTR_CAST(_DST_TYPE, _ptr);
    }
};

/// Simple array iterator. STL like for use with begin(), end(), for(:)
/// also like BOOST_FOREACH
template <class T>
struct cIterator : cPtrFacade<T> {
    typedef cPtrFacade<T> SUPER_t;
    cIterator(T* p) noexcept : SUPER_t(p) {}
    inline void Inc() noexcept {
        this->_ptr++;
    }

    // Prefix increment
    cIterator& operator++() noexcept {
        Inc();
        return *this;
    }
    friend bool operator==(const cIterator& a, const cIterator& b) noexcept {
        return a._ptr == b._ptr;
    }
};
}  // namespace Gray
#endif  // _INC_cPtrFacade_H
