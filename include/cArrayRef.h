//
//! @file cArrayRef.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cArrayRef_H
#define _INC_cArrayRef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cArray.h"
#include "cRefPtr.h"

namespace Gray
{
#define GRAY_FOREACH_S(a,b,c) GRAY_FOREACH( cRefPtr<a>, b, c )

	template<class TYPE>
	class cArrayRef : public cArrayFacade < cRefPtr<TYPE>, TYPE* >
	{
		//! @class Gray::cArrayRef
		//! All items in this array are base on cRefBase. NON sorted.
		//! The array owns a reference to the object like cRefPtr.
		//! Element will get deleted when all references are gone.

		typedef cArrayFacade< cRefPtr<TYPE>, TYPE* > SUPER_t;

	public:
		void DisposeAll()
		{
			//! Similar to RemoveAll() except it calls DisposeThis() to try to dereference all the entries.
			//! ASSUME TYPE supports DisposeThis(); like cXObject
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
