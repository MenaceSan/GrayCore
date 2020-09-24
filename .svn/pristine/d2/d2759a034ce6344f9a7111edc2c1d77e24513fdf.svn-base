//
//! @file CArraySmart.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CArraySmart_H
#define _INC_CArraySmart_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArray.h"
#include "CSmartPtr.h"

namespace Gray
{
#define GRAY_FOREACH_S(a,b,c) GRAY_FOREACH( CSmartPtr<a>, b, c )

	template<class TYPE>
	class CArraySmart : public CArrayFacade < CSmartPtr<TYPE>, TYPE* >
	{
		//! @class Gray::CArraySmart
		//! All items in this array are base on CSmartBase. NON sorted.
		//! The array owns a reference to the object like CSmartPtr.
		//! Element will get deleted when all references are gone.

		typedef CArrayFacade< CSmartPtr<TYPE>, TYPE* > SUPER_t;

	public:
		void DisposeAll()
		{
			//! Similar to RemoveAll() except it calls DisposeThis() to try to dereference all the entries.
			//! ASSUME TYPE supports DisposeThis(); like CXObject
			//! @note often DisposeThis() has the effect of removing itself from the list. Beware of this.

			ITERATE_t iSize = this->GetSize();
			if (iSize <= 0)
				return;
			{	
				// save original list, call DisposeThis on everything from original list. In case Dispose removes itself from the list.
				SUPER_t orig;
				orig.SetCopy(*this);

				ASSERT(orig.GetSize() == iSize);
				for (ITERATE_t i = iSize - 1; i >= 0; i--)	// reverse order they got added?
				{
					TYPE* pObj = orig.GetAt(i);
					if (pObj != nullptr)
					{
						pObj->DisposeThis();
					}
				}
			}
			this->RemoveAll();
		}
	};
}
#endif
