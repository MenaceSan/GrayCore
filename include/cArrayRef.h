//! @file cArrayRef.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArrayRef_H
#define _INC_cArrayRef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cArray.h"
#include "cRefPtr.h"

namespace Gray {
/// <summary>
/// All items in this array are base on cRefBase. NON sorted.
/// The array owns a reference to the object like cRefPtr.
/// Element will get deleted when all references are gone.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cArrayRef : public cArrayFacade<cSpanX<cRefPtr<TYPE>, TYPE*>> {
    typedef cArrayFacade<cSpanX<cRefPtr<TYPE>, TYPE*>> SUPER_t;

 public:
    /// <summary>
    /// Similar to RemoveAll() except it calls DisposeThis() to try to dereference all the entries. Reverse Order!
    /// ASSUME TYPE supports DisposeThis(); like cXObject
    /// @note often DisposeThis() has the effect of removing itself from the list. Beware of this.
    /// </summary>
    void DisposeAll() {
        ITERATE_t iSize = this->GetSize();
        for (ITERATE_t i = iSize - 1; i >= 0; i--) {  // reverse order they got added. might be faster?
            cRefPtr<TYPE> pObj = this->GetAt(i);
            if (pObj != nullptr) pObj->DisposeThis();
            // DisposeThis removes itself from the list?
            const ITERATE_t iSize2 = this->GetSize();
            if (iSize2 != iSize) {
                iSize = iSize2;
                if (i != iSize) i = iSize;  // start over.
            }
        }
        this->RemoveAll();
    }
};
}  // namespace Gray
#endif
