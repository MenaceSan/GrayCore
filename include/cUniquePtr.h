//
//! @file cUniquePtr.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cUniquePtr_H
#define _INC_cUniquePtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cPtrFacade.h"

namespace Gray {
/// <summary>
/// These are sort of dumb "smart pointers" but assume a single reference.
/// A single reference to a dynamically allocated (heap) class not based on cRefBase. Free on destruct.
/// Works like OLD STL "auto_ptr(TYPE)" or new boost::unique_ptr(TYPE), std::unique_ptr(TYPE)
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cUniquePtr : public cPtrFacade<TYPE> { // cNonCopyable 
    friend struct cValArray;
    typedef cUniquePtr<TYPE> THIS_t;
    typedef cPtrFacade<TYPE> SUPER_t;

 public:
    cUniquePtr() noexcept : SUPER_t() {}

    /// <summary>
    /// explicit to make sure we don't copy an allocated pointer accidentally.
    /// </summary>
    explicit cUniquePtr(TYPE* pObj) noexcept : SUPER_t(pObj) {}

    /// <summary>
    /// move constructor is safe.
    /// </summary>
    cUniquePtr(THIS_t&& ref) : SUPER_t(ref) {}

 private:
    /// <summary>
    /// Dangerous! copy would be risky. who free's this?
    /// </summary>
    cUniquePtr(THIS_t& ref) noexcept : cPtrFacade<TYPE>(ref.get_Ptr()) {}

 public:
    void ReleasePtr() noexcept {
        if (this->isValidPtr()) {
            TYPE* p2 = this->get_Ptr();
            this->ClearPtr();  // clear this in case the destructor refs itself in some odd way.
            delete p2;         // assume no throw. noexcept
        }
    }

    ~cUniquePtr() {
        ReleasePtr();
    }

    void AllocArray(size_t nSize = 1) noexcept {
        // ReAlloc ? or just use cArray ??
        ReleasePtr();
        this->AttachPtr(new TYPE[nSize]);
    }
    void AllocArray(size_t nSize, const TYPE* p) noexcept {
        AllocArray(nSize);
        if (p != nullptr && this->isValidPtr()) {
            // Use Copy operator !?!?? or just use cArray ??
            cMem::Copy(this->get_Ptr(), p, sizeof(TYPE) * nSize);
        }
    }

    void AssignPtr(TYPE* p2) noexcept {
        // AKA put_Ptr()?
        if (!this->IsEqual(p2)) {
            ReleasePtr();
            this->AttachPtr(p2);
        }
    }

    // Override operators
    THIS_t& operator=(TYPE* p2) noexcept {
        AssignPtr(p2);
        return *this;
    }
    THIS_t& operator=(THIS_t& ref) noexcept { // Sneaky transfer of ownership!
        if (!this->IsEqual(ref)) {
            ReleasePtr();
            this->AttachPtr(ref.get_Ptr());
            ref.ClearPtr();  // transferred.
        }
        return *this;
    }
    THIS_t& operator=(THIS_t&& ref) noexcept { // move
        if (!this->IsEqual(ref)) {
            ReleasePtr();
            this->AttachPtr(ref.get_Ptr());
            ref.ClearPtr();  // transferred.
        }
        return *this;
    }
};

/// <summary>
/// cUniquePtr Allow a copy constructor that does deep copy.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cUniquePtr2 : public cUniquePtr<TYPE> {
    typedef cUniquePtr2<TYPE> THIS_t;
    typedef cUniquePtr<TYPE> SUPER_t;

 public:
    cUniquePtr2() {}
    cUniquePtr2(const THIS_t& rObj) : cUniquePtr<TYPE>(Dupe(rObj)) {
        //! copy the contents? beware performance problems here. I don't know if its a derived type or array?
    }
    cUniquePtr2(const SUPER_t& rObj) : cUniquePtr<TYPE>(Dupe(rObj)) {
        //! copy the contents? beware performance problems here. I don't know if its a derived type or array?
        //! @note DANGER! Hidden action.
    }
    explicit cUniquePtr2(TYPE* pObj) : cUniquePtr<TYPE>(pObj) {
        // explicit to make sure we don't copy an allocated pointer accidentally.
    }

    static TYPE* Dupe(const SUPER_t& rObj) {
        TYPE* p2 = rObj.get_Ptr();
        if (p2 == nullptr) return nullptr;
        return new TYPE(*p2);
    }

    // Override operators
    THIS_t& operator=(TYPE* p2) {
        this->AssignPtr(p2);
        return *this;
    }
    THIS_t& operator=(const THIS_t& ref) {
        this->AssignPtr(this->Dupe(ref));
        return *this;
    }
};
}  // namespace Gray
#endif  // _INC_cUniquePtr_H
