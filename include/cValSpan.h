//! @file cValSpan.h
//! templates for arrays
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cValSpan_H
#define _INC_cValSpan_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "cValT.h"
#include "cMem.h"
#include <new>  // STL overload the new operator to allow call of constructor directly.

namespace Gray {
/// <summary>
/// Helper functions for array/span of values (cValT) of some TYPE in memory.
/// @note optimizations can be made if we know we are working on larger native types over treating the same things as bytes.
/// </summary>
struct GRAYCORE_LINK cValSpan {  // static. array/span of some TYPE.

    /// <summary>
    /// Difference between 2 pointers in TYPE (not bytes). Check for 64 bit overflow. Safer.
    /// like ptrdiff_t cMem::Diff() but in chars not bytes.
    /// </summary>
    /// <typeparam name="TYPE"></typeparam>
    /// <param name="pszEnd"></param>
    /// <param name="pszStart"></param>
    /// <returns></returns>
    template <class TYPE>
    static ITERATE_t inline Diff(const TYPE* pszEnd, const TYPE* pszStart) {
        ASSERT_NN(pszEnd);
        ASSERT_NN(pszStart);
        const INT_PTR i = pszEnd - pszStart;
        ASSERT(i > -(INT_PTR)(cMem::k_ALLOC_MAX / sizeof(TYPE)) && i < (INT_PTR)(cMem::k_ALLOC_MAX / sizeof(TYPE)));  // k_ALLOC_MAX as TYPE
        return CastN(ITERATE_t, i);
    }

    /// <summary>
    /// fill an array with a repeating TYPE nFillValue.
    /// Ignore negative value for nQty
    /// </summary>
    template <class TYPE>
    static inline void FillQty(TYPE* pArray, ITERATE_t nQty, TYPE nFillValue) noexcept {
        for (ITERATE_t i = 0; i < nQty; i++) {
            pArray[i] = nFillValue;
        }
    }

    /// <summary>
    /// fill an array with a repeating TYPE 0.
    /// </summary>
    template <class TYPE>
    static inline void ZeroQty(TYPE* pArray, ITERATE_t nQty) noexcept {
        cMem::Zero(pArray, nQty * sizeof(TYPE));
    }

    /// <summary>
    /// Forward Copy array of values. like cMem::Copy.
    /// element-copy using class assignment operators for array.
    /// </summary>
    template <class TYPE>
    static inline void CopyQty(TYPE* pDst, const TYPE* pSrc, ITERATE_t nQty) noexcept {
        if (nQty <= 0) return;
        DEBUG_CHECK(!cMem::IsCorruptApp(pDst, nQty * sizeof(TYPE), true));
        DEBUG_CHECK(!cMem::IsCorruptApp(pSrc, nQty * sizeof(TYPE), false));
        for (ITERATE_t i = 0; i < nQty; i++) {
            pDst[i] = pSrc[i];  // use copy operator =.
        }
    }

    /// <summary>
    /// Reverse Copy array of values. like cMem::CopyOverlap.
    /// </summary>
    template <class TYPE>
    static inline void CopyQtyRev(TYPE* pDst, const TYPE* pSrc, ITERATE_t nQty) noexcept {
        for (ITERATE_t i = nQty; i > 0;) {
            i--;
            pDst[i] = pSrc[i];
        }
    }
     
    /// <summary>
    /// Call constructors for an array of elements of _TYPE. For use with cArray
    /// Does not actually allocate memory.
    /// @note we cannot always ASSUME that the constructor actually does anything. (i.e. new int)
    /// for instance void* has no constructor and will be left as CDCDCDCD NOT nullptr as expected.
    /// @note MFC #pragma deprecated ConstructElements name!
    /// </summary>
    template <class TYPE>
    static void __cdecl ConstructElementsX(TYPE* pElements, ITERATE_t nQty) {  // static
        if (nQty <= 0) return;
#ifdef _DEBUG
        DEBUG_ASSERT(!cMem::IsCorruptApp(pElements, nQty * sizeof(TYPE), true), "ConstructElements");
        // for debug. first do bit-wise init (or zero) initialization
        cMem::Fill(pElements, nQty * sizeof(TYPE), cMem::kFillAllocStack);
#endif
        for (; (nQty--) != 0; pElements++) {
            ::new ((void*)pElements) TYPE;  // just call the constructor alone.
        }
    }

    /// <summary>
    /// Call the destructor(s) for an array of _TYPE. Does not explicitly free anything.
    /// @note MFC #pragma deprecated DestructElements name!
    /// </summary>
    template <class TYPE>
    static void __cdecl DestructElementsX(TYPE* pElements, ITERATE_t nQty) {  // static
        if (nQty <= 0) return;

#ifdef _DEBUG
        ASSERT(!cMem::IsCorruptApp(pElements, nQty * sizeof(TYPE), true));
        const ITERATE_t nQtyPrev = nQty;  // just for debug
        UNREFERENCED_PARAMETER(nQtyPrev);
#endif

        for (; (nQty--) != 0; pElements++) {
            (pElements)->~TYPE();
        }
    }

    template <class TYPE>
    static void GRAYCALL Resize(TYPE* pElements, ITERATE_t nNewSize, ITERATE_t nOldSize) {  // static
        if (nNewSize > nOldSize) {
            // initialize the new elements
            cValSpan::ConstructElementsX<TYPE>(&pElements[nOldSize], nNewSize - nOldSize);
        } else if (nOldSize > nNewSize) {
            // destroy the old elements
            cValSpan::DestructElementsX<TYPE>(&pElements[nNewSize], nOldSize - nNewSize);
        }
    }

    /// <summary>
    /// move a single array element from another place. shift the whole array by 1 to make space.
    /// dangerous for types that have internal pointers !
    /// </summary>
    template <class TYPE>
    static void GRAYCALL ShiftElements(TYPE* pFrom, TYPE* pTo) {  // throw
        ptrdiff_t iQty = pTo - pFrom;
        if (iQty == 0) return;
#if 1
        // faster. simple byte mover. no destruct/construct.
        BYTE tmp[sizeof(TYPE)];
        cMem::Copy(tmp, pFrom, sizeof(TYPE));
        // shift old data to fill gap. destroys old pFrom location.
        if (iQty > 0)  // Reverse/back move
            cMem::CopyOverlap(pFrom, pFrom + 1, iQty * sizeof(TYPE));
        else
            cMem::CopyOverlap(pTo + 1, pTo, (-iQty) * sizeof(TYPE));
        // re-init slots we copied from
        cMem::Copy(pTo, tmp, sizeof(TYPE));
#else
        // too slow in debug ?
        TYPE tmp(std::move(*pFrom));
        if (iQty > 0) {  // Reverse/back move
            while (iQty) {
                *pFrom = std::move(pFrom[1]);  // move assignment &&
                pFrom++;
                iQty--;
            }
        } else {  // Forward move
            while (iQty) {
                *pFrom = std::move(pFrom[-1]);  // move assignment &&
                pFrom--;
                iQty++;
            }
        }
        ASSERT(pFrom == pTo);
        *pTo = std::move(tmp);
#endif
    }
};

// Override implementation of simple type templates
template <>
inline void cValSpan::CopyQty<BYTE>(BYTE* pDest, const BYTE* pSrc, ITERATE_t nQty) noexcept {
    //! simple byte copy
    //! Any integral/static type could use this ?
    cMem::Copy(pDest, pSrc, nQty);
}

template <>
inline void cValSpan::FillQty<BYTE>(BYTE* pData, ITERATE_t nQty, BYTE bFill) noexcept {  // static
    //! FillMemory BYTEs like memset()
    cMem::Fill(pData, (size_t)nQty, bFill);
}
}  // namespace Gray
#endif  // _INC_cValSpan_H
