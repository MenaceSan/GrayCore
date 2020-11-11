//
//! @file cListNodeSmart.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cListNodeSmart_H
#define _INC_cListNodeSmart_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CList.h"
#include "cRefPtr.h"
#include "cHeapObject.h"

namespace Gray
{
	template<class _TYPE_REC>
	class cListNodeSmart
	: public cListNodeT < _TYPE_REC >
	, public cRefBase
	{
		//! @class Gray::cListNodeSmart
		//! A smart pointer referenced node in a linked list. Attaching to my parent/list puts a Smart Pointer reference on me.
		//! For Objects that are in CListT and based on cHeapObject,cListNodeBase and cRefBase
		//! @note DecRefCount should "delete this;" in onFinalRelease
		typedef cListNodeT<_TYPE_REC> SUPER_t;
		typedef CListT<_TYPE_REC> PARENT_t;
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
};
#endif // _INC_cListNodeSmart_H
