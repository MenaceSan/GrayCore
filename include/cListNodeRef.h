//
//! @file cListNodeRef.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cListNodeRef_H
#define _INC_cListNodeRef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cList.h"
#include "cRefPtr.h"
#include "cHeapObject.h"

namespace Gray
{
	template<class _TYPE_REC>
	class cListNodeRef
		: public cListNodeT < _TYPE_REC >
		, public cRefBase
		, public cHeapObject
	{
		//! @class Gray::cListNodeRef
		//! A smart pointer to reference counted node in a linked list. Attaching to my parent/list puts a Smart Pointer reference on me.
		//! For Objects that are in cListT and based on cHeapObject, cListNodeBase and cRefBase
		//! @note cRefBase DecRefCount should "delete this;" in onFinalRelease

		typedef cListNodeT<_TYPE_REC> SUPER_t;
		typedef cListT<_TYPE_REC> PARENT_t;

	protected:
		virtual void put_Parent(PARENT_t* pParent)
		{
			//! being in the list acts like a reference.
			SUPER_t::put_Parent(pParent);
			if (pParent != nullptr)
				IncRefCount();
			else
				DecRefCount();
		}
	};
}
#endif // _INC_cListNodeRef_H
