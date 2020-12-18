//
//! @file Ptr.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_Ptr_H
#define _INC_Ptr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"

namespace Gray
{

#ifdef USE_64BIT
#define _SIZEOF_PTR 8	//!< bytes = sizeof(void*) for __DECL_ALIGN macro. Can't do sizeof(x). uintptr_t
#define USE_FILE_POS64
#else
#define _SIZEOF_PTR 4	//!< bytes = sizeof(void*) for __DECL_ALIGN macro. Can't do sizeof(x). uintptr_t
#endif

#ifdef _CPPRTTI
#define DYNPTR_CAST(t,p)		(dynamic_cast <t*>(p))		// typically used for safer down-casting.
#define IS_TYPE_OF(t,p)			((dynamic_cast<const t*>(p))!=nullptr)
#else
#define DYNPTR_CAST(t,p)		(static_cast <t*>(p))	// dynamic_cast isn't available ?
#define IS_TYPE_OF(t,p)			true	//! @todo no _CPPRTTI so use CObject for this like MFC IsKindOf() does ?
#endif

	template <class _TYPE_TO, class _TYPE_FROM >
	inline bool is_valid_cast(_TYPE_FROM* p)
	{
		// will dynamic_cast work ? AKA IS_TYPE_OF() ?
		// TODO typeid(p) == typeid(_TYPE_TO); as dynamic? or IS_TYPE_OF(_TYPE_TO)
		if (p == nullptr)
			return true;
		_TYPE_TO* p2 = dynamic_cast<_TYPE_TO*>(p);
		return p2 != nullptr ;
	}

	template <class _TYPE_TO, class _TYPE_FROM >
	inline _TYPE_TO* check_cast(_TYPE_FROM* p)
	{
		//! like static_cast<>() but with some extra checking.
		//! up-casting can be dangerous. We assume it is safe in this case but check it anyhow. 
		//! a static_cast that gets verified in _DEBUG mode. fast normal static_cast in release mode.
		//! nullptr is valid.
#ifdef _DEBUG
		ASSERT(is_valid_cast<_TYPE_TO>(p));
#endif
		return static_cast<_TYPE_TO*>(p);
	};

#define CHECKPTR_CAST(t,p)	(::Gray::check_cast<t>(p))
}

#endif
