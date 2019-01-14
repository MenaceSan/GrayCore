//
//! @file CListNodeSmart.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CListNodeSmart_H
#define _INC_CListNodeSmart_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CList.h"
#include "CSmartPtr.h"
#include "CHeapObject.h"

namespace Gray
{
	template<class _TYPE_REC>
	class CListNodeSmart
	: public CListNodeT < _TYPE_REC >
	, public CSmartBase
	{
		//! @class Gray::CListNodeSmart
		//! A smart pointer referenced node in a list. Attaching to my parent/list puts a Smart Pointer reference on me.
		//! For Objects that are in CListT and based on CHeapObject,CListNodeBase and CSmartBase
		//! @note DecRefCount should "delete this;" in onFinalRelease
		typedef CListNodeT<_TYPE_REC> SUPER_t;
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
#endif // _INC_CListNodeSmart_H
