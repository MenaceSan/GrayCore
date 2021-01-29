//
//! @file Index.h
//! Difference of 2 pointers in memory.
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


