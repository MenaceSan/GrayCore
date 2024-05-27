//! @file cHeapObject.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cHeapObject_H
#define _INC_cHeapObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "IUnknown.h"  // DECLARE_INTERFACE
#include "cHeap.h"

#if defined(_DEBUG) || defined(_DEBUG_FAST)
#define USE_HEAPSIG
#endif

namespace Gray {
/// <summary>
/// This is a base interface supported by objects/classes that are assumed allocated on the heap.
/// Use this because multiple inheritance can hide my top heap (free-able) pointer.
/// Top should implement some version of cHeapObject. e.g. "x = new cXObject"
/// </summary>
DECLARE_INTERFACE(IHeapObject) {
    IGNORE_WARN_INTERFACE(IHeapObject);
    virtual const void* get_HeapPtr() const noexcept = 0;  /// Get the top level (outermost, free-able) class pointer. I can delete get_HeapPtr().
};

//*************************************************

/// <summary>
/// The base of some class/struct object that is ALWAYS heap allocated.
/// This item MUST always be dynamically allocated with new/delete!
/// Never stack (auto) or data segment (static) based.
/// get_HeapPtr() must be declared by the highest level ! since derived classes wrap their base classes.
/// multiple inheritance can disguise the base allocated pointer.
/// May need cHeapAlign.
/// </summary>
class GRAYCORE_LINK cHeapObject : public IHeapObject {
 protected:
#ifdef USE_HEAPSIG
    cMemSignature<0xcab005e> _Sig;  /// may want to have multiple of these ? (or none)
#endif

    // Add this to each IHeapObject rooted object to get the base heap allocation pointer. Avoids problems with multiple inheritance and heap allocated objects.
#define CHEAPOBJECT_IMPL \
    const void* get_HeapPtr() const noexcept override { return this; }

 public:
    /// <summary>
    /// @note virtuals do not work in destructor ! get_HeapPtr? so isValidCheck() not possible here !
    /// </summary>
    virtual ~cHeapObject() {
        ASSERT(_Sig.isValidSignature());
    }

    /// <summary>
    /// Get the top level "new" pointer in the case of multiple inheritance. CHEAPOBJECT_IMPL;
    /// </summary>
    /// <returns></returns>
    const void* get_HeapPtr() const noexcept override {
        return this;
    }

    /// Is index a valid offset inside this object?
    bool IsValidInsideN(INT_PTR index) const noexcept {
#ifdef USE_HEAPSIG
        if (!_Sig.isValidSignature()) return false;
#endif
        const void* pBase = get_HeapPtr();
        return cHeapAlign::IsValidInside(pBase, index);
    }

    /// Is pTest a valid pointer inside the this object ?
    bool IsValidInsidePtr(void const* pTest) const noexcept {
        if (pTest == nullptr) return false;
#ifdef USE_HEAPSIG
        if (!_Sig.isValidSignature()) return false;
#endif
        const void* pBase = get_HeapPtr();
        return cHeapAlign::IsValidInside(pBase, cMem::Diff(pTest, pBase));
    }
    /// <summary>
    /// Get size of *this as opposed to size of children.
    /// may be called by CountHeapStats()
    /// </summary>
    /// <param name="iAllocCount"></param>
    /// <returns></returns>
    virtual size_t GetHeapStatsThis(OUT ITERATE_t& iAllocCount) const {
#ifdef USE_HEAPSIG
        ASSERT(_Sig.isValidSignature());
#endif
        iAllocCount++;
        return cHeapAlign::GetSize(get_HeapPtr());
    }
    virtual bool isValidCheck() const noexcept {
        //! @note can't call this in a destructor since get_HeapPtr() is virtual.
        if (!cMem::IsValidApp(this))  // NOT be based on nullptr ? sanity check.
            return false;
#ifdef USE_HEAPSIG
        if (!_Sig.isValidSignature()) return false;
#endif
        if (!cHeapAlign::IsValidHeap(get_HeapPtr()))  // might be aligned.
            return false;
        return true;
    }
};

// Create an ^2 aligned pool for allocation of these objects. C++17 doesnt need this? It has std::align_val_t that is honored by new ?
// Used with __DECL_ALIGN(_SIZEOF_PTR) like XM_ALIGNED_DATA,
#define DECLARE_HEAP_ALIGNED_ALLOC(_CLASS)                                                                         \
 public:                                                                                                           \
    static void* operator new(size_t sizeOfClass) { return cHeapAlign::AllocPtr(sizeOfClass, __alignof(_CLASS)); } \
    static void operator delete(void* pData) { cHeapAlign::FreePtr(pData); }
}  // namespace Gray

#endif  // _INC_cHeapObject_H
