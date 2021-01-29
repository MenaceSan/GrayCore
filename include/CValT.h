//
//! @file cValT.h
//! templates for comparing, swapping and sorting of any type.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cValT_H
#define _INC_cValT_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"

namespace Gray
{
	typedef int COMPARE_t;	//!< result of compare. 0=same, 1=a>b, -1=a<b
	enum COMPARE_TYPE
	{
		//! @enum Gray::COMPARE_TYPE
		//! General return type from a compare. Similar to _WIN#2 VARCMP_GT
		COMPARE_Less = -1,		//!< VARCMP_LT
		COMPARE_Equal = 0,		//!< VARCMP_EQ
		COMPARE_Greater = 1,	//!< VARCMP_GT
	};

	struct GRAYCORE_LINK cValT	// static. Value/Object of some type in memory.
	{
		//! @struct Gray::cValT
		//! Helper functions for an arbitrary value/object type in memory. We may compare these.
		//! Similar to System.IComparable in .NET

		template <class TYPE>
		static inline void Swap(TYPE& a, TYPE& b) noexcept
		{
			//! swap 2 values. similar to cMem::Swap() but uses the intrinsic = operator.
			//! dangerous for complex struct that has pointers and such. may not do a 'deep' copy.
			//! assume TYPE has a safe overloaded = operator.
			//! Overload this template for any specific TYPE Swaps.
			register TYPE x = a;
			a = b;
			b = x;
		}

		template <class TYPE>
		static inline COMPARE_t Compare(const TYPE& a, const TYPE& b) noexcept
		{
			//! compare 2 TYPE values.
			//! Similar to .NET IComparable but for any types.
			//! Overload this template for any specific TYPE Compare.
			//! obviously TYPE must also support >. We assume all types already support ==.
			//! @note we need this because INT_MAX-INT_MIN is not positive !!! (and 0-0xFFFFFFFF is not negative)
			//! @note also memcmp() is a backwards numeric compare for USE_LITTLE_ENDIAN (Intel) machines.
			if (a > b)
				return COMPARE_Greater; // is greater than.
			if (a == b)
				return COMPARE_Equal;	// is equal.
			return COMPARE_Less;	// must be less than.
		}
	};

	struct GRAYCORE_LINK cValArray	// static. array of Value of some TYPE.
	{
		//! @struct Gray::cValArray
		//! Helper functions for array of values of some TYPE in memory.
		//! @note optimizations can be made if we know we are working on larger native types over treating the same things as bytes.

		template <class TYPE>
		static inline bool IsFilledQty(const TYPE* pArray, ITERATE_t nBlocks, TYPE nFillValue) noexcept
		{
			//! Is this array filled with a repeating value ?
			for (ITERATE_t i = 0; i < nBlocks; i++)
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
		static inline bool IsEqualQty(const TYPE* pArray1, const TYPE* pArray2, ITERATE_t nBlocks) noexcept
		{
			//! Test 2 arrays of a TYPE. like memcmp.
			for (ITERATE_t i = 0; i < nBlocks; i++)
			{
				if (pArray1[i] != pArray2[i])
					return false;
			}
			return true;
		}

		template <class TYPE>
		static inline COMPARE_t CompareQty(const TYPE* pArray1, const TYPE* pArray2, ITERATE_t nBlocks) noexcept
		{
			//! Compare 2 arrays of a TYPE. like memcmp.
			for (ITERATE_t i = 0; i < nBlocks; i++)
			{
				if (pArray1[i] != pArray2[i])
					return (COMPARE_t)(pArray1[i] - pArray2[i]);
			}
			return COMPARE_Equal;
		}

		template <class TYPE>
		static inline void FillQty(TYPE* pArray, ITERATE_t nBlocks, TYPE nFillValue=0) noexcept
		{
			//! fill an array with a repeating TYPE nFillValue.
			//! Ignore negative value for nBlocks

			for (ITERATE_t i = 0; i < nBlocks; i++)
			{
				pArray[i] = nFillValue;
			}
		}
		template <class TYPE>
		static inline void ZeroQty(TYPE* pArray, ITERATE_t nBlocks) noexcept
		{
			//! fill an array with a repeating TYPE 0.
			for (ITERATE_t i = 0; i < nBlocks; i++)
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
		static inline void CopyQty(TYPE* pDst, const TYPE* pSrc, ITERATE_t nBlocks) noexcept
		{
			//! Forward Copy array of values. like memcpy.
			for (ITERATE_t i = 0; i < nBlocks; i++)
			{
				pDst[i] = pSrc[i];
			}
		}

		template <class TYPE>
		static inline void CopyQtyRev(TYPE* pDst, const TYPE* pSrc, ITERATE_t nBlocks) noexcept
		{
			//! Reverse Copy array of values. like memmove.
			for (ITERATE_t i = nBlocks; i > 0;)
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
		static void GRAYCALL ReverseArrayBlocks(void* pArray, size_t nArraySizeBytes, size_t iBlockSize);
	};

	template <>
	inline void cValArray::FillQty<BYTE>(BYTE* pData, ITERATE_t nBlocks, BYTE bFill) noexcept // static
	{
		//! FillMemory BYTEs like memset()
		::memset(pData, bFill, (size_t)nBlocks);
	}
	template <>
	inline void cValArray::FillSize<BYTE>(void* pData, size_t nSizeBlock, BYTE bFill) noexcept // static
	{
		//! FillMemory BYTEs like memset()
		::memset(pData, bFill, nSizeBlock);
	}
}

#endif // _INC_cValT_H
