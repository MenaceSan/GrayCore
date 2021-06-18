//
//! @file cValArray.h
//! templates for arrays
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cValArray_H
#define _INC_cValArray_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "cHeap.h"
#include <new>	// STL overload the new operator to allow call of constructor directly.

namespace Gray
{
	struct GRAYCORE_LINK cValArray	// static. array of Value of some TYPE.
	{
		//! @struct Gray::cValArray
		//! Helper functions for array of values of some TYPE in memory.
		//! @note optimizations can be made if we know we are working on larger native types over treating the same things as bytes.

		template <class TYPE>
		static inline bool IsFilledQty(const TYPE* pArray, ITERATE_t nQty, TYPE nFillValue) noexcept
		{
			//! Is this array filled with a repeating value ?
			for (ITERATE_t i = 0; i < nQty; i++)
			{
				if (pArray[i] != nFillValue)
					return false;
			}
			return true;
		}
		template <class TYPE>
		static inline bool IsFilledSize(const void* pArray, size_t nArraySizeBytes, TYPE nFillValue) noexcept
		{
			//! Is this array filled with a repeating value ? _countof()
			return IsFilledQty((const TYPE*)pArray, (ITERATE_t)(nArraySizeBytes / sizeof(TYPE)), nFillValue);
		}

		template <class TYPE>
		static inline bool IsEqualQty(const TYPE* pArray1, const TYPE* pArray2, ITERATE_t nQty) noexcept
		{
			//! Test 2 arrays of a TYPE. like cMem::Compare.
			if (pArray1 == pArray2)
				return true;
			for (ITERATE_t i = 0; i < nQty; i++)
			{
				if (!(pArray1[i] == pArray2[i])) // Assume everything supports the == operator.
					return false;
			}
			return true; // looks the same to me.
		}

		template <class TYPE>
		static inline COMPARE_t CompareQty(const TYPE* pArray1, const TYPE* pArray2, ITERATE_t nQty) noexcept
		{
			//! Compare 2 arrays of a TYPE. like cMem::Compare.
			if (pArray1 == pArray2)
				return COMPARE_Equal;
			for (ITERATE_t i = 0; i < nQty; i++)
			{
				if (!(pArray1[i] == pArray2[i])) // Assume everything supports the == operator.
					return (COMPARE_t)(pArray1[i] - pArray2[i]);
			}
			return COMPARE_Equal;
		}

		template <class TYPE>
		static inline void FillQty(TYPE* pArray, ITERATE_t nQty, TYPE nFillValue = 0) noexcept
		{
			//! fill an array with a repeating TYPE nFillValue.
			//! Ignore negative value for nQty

			for (ITERATE_t i = 0; i < nQty; i++)
			{
				pArray[i] = nFillValue;
			}
		}
		template <class TYPE>
		static inline void ZeroQty(TYPE* pArray, ITERATE_t nQty) noexcept
		{
			//! fill an array with a repeating TYPE 0.
			for (ITERATE_t i = 0; i < nQty; i++)
			{
				pArray[i] = 0;
			}
		}

		template <class TYPE>
		static inline void FillSize(void* pArray, size_t nArraySizeBytes, TYPE nFillValue) noexcept
		{
			//! Fill a block of memory with a repeating TYPE nFillValue by size_t not quantity.
			//! Similar to the native memset() FillMemory
			//! If TYPE is not BYTE this may leave unaligned block at the end.
			FillQty((TYPE*)pArray, (ITERATE_t)(nArraySizeBytes / sizeof(TYPE)), nFillValue);
		}

		template <class TYPE>
		static inline void CopyQty(TYPE* pDst, const TYPE* pSrc, ITERATE_t nQty) noexcept
		{
			//! Forward Copy array of values. like cMem::Copy.
			//! element-copy using class assignment operators for array.

			if (nQty <= 0)
				return;

			ASSERT(!cMem::IsCorrupt(pDst, nQty * sizeof(TYPE), true));
			ASSERT(!cMem::IsCorrupt(pSrc, nQty * sizeof(TYPE), false));

			for (ITERATE_t i = 0; i < nQty; i++)
			{
				pDst[i] = pSrc[i];	// use copy operator =.
			}
		}

		template <class TYPE>
		static inline void CopyQtyRev(TYPE* pDst, const TYPE* pSrc, ITERATE_t nQty) noexcept
		{
			//! Reverse Copy array of values. like cMem::CopyOverlap.
			for (ITERATE_t i = nQty; i > 0;)
			{
				i--;
				pDst[i] = pSrc[i];
			}
		}

		template <class TYPE>
		static inline void ReverseArray(TYPE* pArray, size_t nArraySizeBytes) noexcept
		{
			//! reverse the order of an array of a TYPE. similar to cMem::ReverseArrayBlocks but iBlockSize==sizeof(TYPE)
			//! TYPE = BYTE = reverse bytes in a block. similar to htonl(), etc.
			//! Use ReverseType<> if TYPE = BYTE and nArraySizeBytes <= largest intrinsic type size?
			size_t nQty = nArraySizeBytes / sizeof(TYPE);
			TYPE* pMemBS = pArray;
			TYPE* pMemBE = pArray + (nQty - 1);
			for (; pMemBS < pMemBE; pMemBS++, pMemBE--)
			{
				cValT::Swap<TYPE>(*pMemBS, *pMemBE);
			}
		}

		template <class TYPE>
		static void __cdecl ConstructElementsX(TYPE* pElements, ITERATE_t nQty) // static
		{
			//! Call constructors for an array of elements of _TYPE. For use with CArray
			//! Does not actually allocate memory.
			//! @note we cannot always ASSUME that the constructor actually does anything. (i.e. new int)
			//! for instance void* has no constructor and will be left as CDCDCDCD NOT nullptr as expected.
			//! @note MFC #pragma deprecated ConstructElements name!
			if (nQty <= 0)
				return;

			DEBUG_ASSERT(!cMem::IsCorrupt(pElements, nQty * sizeof(TYPE), true), "ConstructElements");

#ifdef _DEBUG
			// for debug. first do bit-wise init (or zero) initialization
			cValArray::FillSize<BYTE>(pElements, nQty * sizeof(TYPE), cHeap::FILL_Alloc);
#endif

			for (; (nQty--) != 0; pElements++)
			{
				::new((void*)pElements) TYPE;	// just call the constructor alone.
			}
		}

		template <class TYPE>
		static void __cdecl DestructElementsX(TYPE* pElements, ITERATE_t nQty) // static
		{
			//! Call the destructor(s) for an array of _TYPE. Does not explicitly free anything.
			//! @note MFC #pragma deprecated DestructElements name!

			if (nQty <= 0)
				return;

#ifdef _DEBUG
			ASSERT(!cMem::IsCorrupt(pElements, nQty * sizeof(TYPE), true));
			const ITERATE_t nQtyPrev = nQty;	// just for debug
			UNREFERENCED_PARAMETER(nQtyPrev);
#endif

			for (; (nQty--) != 0; pElements++)
			{
				(pElements)->~TYPE();
			}
		}

		template <class TYPE>
		static void GRAYCALL Resize(TYPE* pElements, ITERATE_t nNewSize, ITERATE_t nOldSize) // static
		{
			if (nNewSize > nOldSize)
			{
				// initialize the new elements
				cValArray::ConstructElementsX<TYPE>(&pElements[nOldSize], nNewSize - nOldSize);
			}
			else if (nOldSize > nNewSize)
			{
				// destroy the old elements
				cValArray::DestructElementsX<TYPE>(&pElements[nNewSize], nOldSize - nNewSize);
			}
		}

		template <class TYPE>
		static void GRAYCALL MoveElement1(TYPE* pFrom, TYPE* pTo) // throw
		{
			//! move an index from another place. shift the whole array by 1 to make space.
			//! dangerous for types that have internal pointers !
			BYTE bTmp[sizeof(TYPE)];
			cMem::Copy(bTmp, pFrom, sizeof(TYPE));
			// shift old data to fill gap
			cMem::CopyOverlap(pTo + 1, pTo, (pFrom - pTo) * sizeof(TYPE));	// destroys old pFrom location.
			// re-init slots we copied from
			cMem::Copy(pTo, bTmp, sizeof(TYPE));
		}

		static void GRAYCALL ReverseArrayBlocks(void* pArray, size_t nArraySizeBytes, size_t iBlockSize);
	};

	// Override implementation of templates
	template <>
	inline void __cdecl cValArray::CopyQty<BYTE>(BYTE* pDest, const BYTE* pSrc, ITERATE_t nQty)
	{
		//! simple byte copy
		//! Any integral/static type could use this ?
		cMem::Copy(pDest, pSrc, nQty);
	}

	template <>
	inline void cValArray::FillQty<BYTE>(BYTE* pData, ITERATE_t nQty, BYTE bFill) noexcept // static
	{
		//! FillMemory BYTEs like memset()
		cMem::Fill(pData, (size_t)nQty, bFill);
	}
	template <>
	inline void cValArray::FillSize<BYTE>(void* pData, size_t nSizeBlock, BYTE bFill) noexcept // static
	{
		//! FillMemory BYTEs like memset()
		cMem::Fill(pData, nSizeBlock, bFill);
	}
}

#endif // _INC_cValArray_H
