//
//! @file Index.h
//! Difference of 2 pointers in memory.
//! templates for comparing, swapping of any type.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_Index_H
#define _INC_Index_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

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

#ifdef USE_64BIT
#define _SIZEOF_PTR 8	//!< bytes = sizeof(void*) for __DECL_ALIGN macro. Can't do sizeof(x). uintptr_t
#define USE_FILE_POS64
#else
#define _SIZEOF_PTR 4	//!< bytes = sizeof(void*) for __DECL_ALIGN macro. Can't do sizeof(x). uintptr_t
#endif

#if (_MFC_VER > 0x0600)
	typedef INT_PTR ITERATE_t;		//!< MFC 6 uses INT_PTR for array indexes.
#else
	typedef int ITERATE_t;		//!< like size_t but signed
#endif
	const ITERATE_t k_ITERATE_BAD = -1;

	typedef size_t COUNT_t;		//!< like size_t but a count of things that might NOT be bytes. ASSUME unsigned. _countof(x)

#define IS_INDEX_BAD(i,q)		((COUNT_t)(i)>=(COUNT_t)(q))	//!< cast the (likely) int to unsigned to check for negatives.
#define IS_INDEX_GOOD(i,q)		((COUNT_t)(i)<(COUNT_t)(q))		//!< cast the (likely) int to unsigned to check for negatives.

#define IS_INDEX_BAD_ARRAY(i,a)		IS_INDEX_BAD(i,_countof(a))
#define IS_INDEX_GOOD_ARRAY(i,a)	IS_INDEX_GOOD(i,_countof(a))

	template< typename TYPE >
	static inline ptrdiff_t GET_INDEX_IN(TYPE a, TYPE b)
	{
		//! diff 2 pointers of the same type to get index diff. 
		//! INT_PTR is same type as intptr_t, ITERATE_t or ptrdiff_t ?
		//! Assume b is an element in array a.
		return b - a;
	}

#define _sizeofm(s,m)	sizeof(((s *)0)->m)	//!< size_t of a structure member/field (like offsetof()) nullptr

}
#endif
