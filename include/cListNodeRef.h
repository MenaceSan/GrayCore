//! @file cListNodeRef.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cListNodeRef_H
#define _INC_cListNodeRef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cHeapObject.h"
#include "cList.h"
#include "cRefPtr.h"

namespace Gray {
/// <summary>
/// A smart pointer to reference counted node in a linked list. Attaching to my parent/list puts a Smart Pointer reference on me.
/// For Objects that are in cListT and based on cListNode and cRefBase and probably cHeapObject
/// @note cRefBase DecRefCount should "delete this;" in onZeroRefCount()
/// </summary>
/// <typeparam name="_TYPE_REC"></typeparam>
template <class _TYPE_REC>
class cListNodeRef : public cListNodeT<_TYPE_REC>, public cRefBase {
    typedef cListNodeT<_TYPE_REC> SUPER_t;
    typedef cListT<_TYPE_REC> PARENT_t;

 protected:
    void OnChangeListParent(cList* pParent) override {
        //! being in the list acts like a reference.
        SUPER_t::OnChangeListParent(pParent);
        if (pParent != nullptr)
            IncRefCount();
        else
            DecRefCount(); // i MAY get deleted by this!
    }
};
}  // namespace Gray
#endif  // _INC_cListNodeRef_H
