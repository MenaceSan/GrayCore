//
//! @file PtrCast.h
//! A pointer to some struct or class. Not used for pointers to basic/intrinsic types.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_PtrCast_H
#define _INC_PtrCast_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"

namespace Gray
{

#ifdef _CPPRTTI
#define DYNPTR_CAST(t,p)		(dynamic_cast <t*>(p))		// typically used for safer down-casting.
#define IS_TYPE_OF(t,p)			((dynamic_cast<const t*>(p))!=nullptr)
#else
#define DYNPTR_CAST(t,p)		(static_cast <t*>(p))	// dynamic_cast isn't available ?
#define IS_TYPE_OF(t,p)			true	//! @todo no _CPPRTTI so use CObject for this like MFC IsKindOf() does ?
#endif

	// https://stackoverflow.com/questions/1785426/c-sharp-null-coalescing-operator-equivalent-for-c
	// beware a might be evaluated twice ? // dont evaluate b if a is nullptr. So leave this as macro. null coalesce. 
#define SAFE_PROPN(a,b)		(((a)!=nullptr)?((a)->b) : nullptr)
#define SAFE_PROP(a,b,c)	(((a)!=nullptr)?((a)->b) : (c))		// AKA NULL_PROP() ?

	template <class _TYPE_TO, class _TYPE_FROM >
	inline bool is_valid_cast(_TYPE_FROM* p)
	{
		//! will dynamic_cast work ? AKA IS_TYPE_OF() ? like std::is_base_of()
		//! TODO typeid(p) == typeid(_TYPE_TO); as dynamic? or IS_TYPE_OF(_TYPE_TO)
		if (p == nullptr)	// nullptr is always castable.
			return true;	
		_TYPE_TO* p2 = dynamic_cast<_TYPE_TO*>(p);
		return p2 != nullptr ;
	}

	template <class _TYPE_TO, class _TYPE_FROM >
	inline _TYPE_TO* check_cast(_TYPE_FROM* p)
	{
		//! like static_cast<>() but with some extra checking. // dynamic for DEBUG only.
		//! up-casting can be dangerous. We assume it is safe in this case but check it anyhow. 
		//! a static_cast that gets verified in _DEBUG mode. fast normal static_cast in release mode.
		//! nullptr is valid.
#ifdef _DEBUG
		ASSERT(is_valid_cast<_TYPE_TO>(p));
#endif
		return static_cast<_TYPE_TO*>(p);
	};

#define CHECKPTR_CAST(t,p)	(::Gray::check_cast<t>(p))	// dynamic for DEBUG only.
}

#endif
