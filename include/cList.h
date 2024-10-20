//! @file cList.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cList_H
#define _INC_cList_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "HResult.h"
#include "cDebugAssert.h"
#include "cHeapObject.h"

namespace Gray {
class cList;

/// <summary>
/// abstract base class for a single node/element in a Double linked cList.
/// @NOTE Single owner = This item belongs to JUST ONE cList (_pParent)
/// derive a class from cListNode to be a node member in a cList.
/// NOT circular. Node head/prev and tail/next are nullptr.
/// </summary>
class GRAYCORE_LINK cListNode : public cHeapObject {
    friend cList;                  // so _pNext and _pPrev can be manipulated directly.
    cList* _pParent = nullptr;    /// link me back to my parent object.
    cListNode* _pNext = nullptr;  /// next sibling
    cListNode* _pPrev = nullptr;  /// previous sibling

 protected:
    cListNode() noexcept {}

    /// <summary>
    /// I am being added to a list. (or nullptr = no list)
    /// DO NOT remove from list inside this call!
    /// </summary>
    /// <param name="pParent"></param>
    virtual void OnChangeListParent(cList* pParent) {
        ASSERT(_pParent == nullptr || pParent == nullptr || _pParent == pParent);
        _pParent = pParent;  // link me to my list parent object.
    }

 public:
    virtual ~cListNode() {
        //! ASSUME: RemoveFromParent() was already called! (virtuals don't work in destruct!)
        DEBUG_CHECK(!hasParent());
    }

    cList* get_Parent() const noexcept {
        return _pParent;
    }
    cListNode* get_Next() const noexcept {
        return _pNext;
    }
    cListNode* get_Prev() const noexcept {
        return _pPrev;
    }

    /// <summary>
    /// is this in a list? If i have no parent i shouldn't have any siblings either.
    /// </summary>
    bool hasParent() const noexcept {
        if (_pParent != nullptr) return true;
        DEBUG_CHECK(_pNext == nullptr && _pPrev == nullptr);
        return false;
    }

    /// <summary>
    /// Remove this ListNode(myself) from my parent list (if i have one).
    /// </summary>
    void RemoveFromParent();

    /// <summary>
    /// delete myself from the system.
    /// Pre-destructor cleanup things can be done since virtual(s) don't work in destructor(s).
    /// @note this does not free the memory. override to do this.
    /// </summary>
    virtual HRESULT DisposeThis() {
        RemoveFromParent();  // called before destruct so virtual RemoveListNode() is called correctly.
        return S_OK;
    }
};

//*************************************************

/// <summary>
/// generic Double linked list container. NOT circular. head and tail are nullptr.
/// @note Lists are primarily used where inserts and deletes for large sets occurs frequently.
/// @note cListNode can ONLY belong to one single cList.
/// Objects should remove themselves from the list when deleted.
/// Similar to the MFC CList, or std::list(T), std::deque
/// </summary>
class GRAYCORE_LINK cList {
    friend class cListNode;        // so it can call RemoveListNode() to remove self.
    cListNode* _pHead = nullptr;  /// Head of my list.
    cListNode* _pTail = nullptr;  /// Tail of my list.

 protected:
    ITERATE_t _nCount = 0;  /// how many children? nice to get read only direct access to this for scripting.

 protected:
    void ClearList() noexcept {
        DEBUG_ASSERT(_nCount == 0, "List not cleaned up properly!");
        _pHead = nullptr;
        _pTail = nullptr;
        _nCount = 0;
    }

    //! Override this to get called when an item is removed from this list.
    //! Never called directly. ALWAYS called from pObRec->RemoveFromParent()
    virtual void RemoveListNode(cListNode* pNode);  /// allow Override of this. called when child removed from list.

 public:
    virtual ~cList() {
        // @note virtuals do not work in destructor!
        // ASSUME: DisposeAll() or Empty() is called from higher levels
        //  it is important for virtual RemoveListNode() callback
        DEBUG_CHECK(isEmptyList());
    }

    cListNode* get_Head() const noexcept {
        return _pHead;
    }
    cListNode* get_Tail() const noexcept {
        return _pTail;
    }
    ITERATE_t get_Count() const noexcept {
        return _nCount;
    }
    bool isEmptyList() const noexcept {
        return _nCount == 0;
    }
    bool IsMyChild(const cListNode* pNode) const noexcept {
        return pNode != nullptr && pNode->get_Parent() == this;
    }

    /// <summary>
    /// iterate/enumerate the linked list.
    /// Not very efficient.
    /// </summary>
    /// <returns>nullptr = past end of list.</returns>
    /// <param name="index"></param>
    cListNode* GetAt(ITERATE_t index) const noexcept;

    /// <summary>
    /// Override this to check for items being added.
    /// </summary>
    /// <param name="pNodeNew"></param>
    /// <param name="pNodePrev">nullptr = first</param>
    virtual void InsertListNode(cListNode* pNodeNew, cListNode* pNodePrev = nullptr);

    /// <summary>
    /// Transfer ALL the contents of another list pListSrc into this one.
    /// </summary>
    /// <param name="pListSrc"></param>
    /// <param name="pNodePrev">Node from THIS list</param>
    void MoveListNodes(cList* pListSrc, cListNode* pNodePrev = nullptr);

    /// <summary>
    /// Insert in some order in the list.
    /// </summary>
    /// <param name="pNodeNew"></param>
    /// <param name="pNodeNext">nullptr = InsertTail</param>
    void InsertBefore(cListNode* pNodeNew, const cListNode* pNodeNext) {
        InsertListNode(pNodeNew, (pNodeNext != nullptr) ? (pNodeNext->get_Prev()) : get_Tail());
    }
    void InsertHead(cListNode* pNodeNew) {
        InsertListNode(pNodeNew, nullptr);
    }
    void InsertTail(cListNode* pNodeNew) {
        InsertListNode(pNodeNew, get_Tail());
    }

    /// <summary>
    /// call DisposeThis() for all entries.
    /// </summary>
    void DisposeAll();

    /// <summary>
    /// empty the list. but don't necessarily DisposeThis() the objects.
    /// </summary>
    void SetEmptyList();
};

//*************************************************

inline void cListNode::RemoveFromParent() {
    if (_pParent == nullptr) return;
    _pParent->RemoveListNode(this);  // only call this from here !
    // ASSERT( _pParent != pParentPrev );	// We are now unlinked. (or deleted)
}

//*************************************************
// template type casting for lists.

/// <summary>
/// Assume this is a node (in a linked list) of type _TYPE_REC. e.g. _TYPE_REC is based on cListNodeT which is based on cListNode.
/// </summary>
/// <typeparam name="_TYPE_REC"></typeparam>
template <class _TYPE_REC = cListNode>
struct cListNodeT : public cListNode {
    typedef cListNode SUPER_t;
    /// get_Next cast to _TYPE_REC
    _TYPE_REC* get_Next() const {
        return static_cast<_TYPE_REC*>(SUPER_t::get_Next());
    }
    /// get_Prev cast to _TYPE_REC
    _TYPE_REC* get_Prev() const {
        return static_cast<_TYPE_REC*>(SUPER_t::get_Prev());
    }
};

/// <summary>
/// Hold a List of _TYPE_REC things.
/// @note _TYPE_REC is the type of class this list contains. _TYPE_REC is based on cListNodeT<_TYPE_REC> and/or cListNode
/// </summary>
/// <typeparam name="_TYPE_REC"></typeparam>
template <class _TYPE_REC = cListNode>
struct cListT : public cList {
    typedef cList SUPER_t;
    typedef cListNodeT<_TYPE_REC> NODEBASE_t;

    /// <summary>
    /// iterate/enumerate the linked list.
    /// </summary>
    _TYPE_REC* GetAt(ITERATE_t index) const {
        return static_cast<_TYPE_REC*>(SUPER_t::GetAt(index));
    }
    _TYPE_REC* get_Head() const {
        return static_cast<_TYPE_REC*>(SUPER_t::get_Head());
    }
    _TYPE_REC* get_Tail() const {
        return static_cast<_TYPE_REC*>(SUPER_t::get_Tail());
    }
};
}  // namespace Gray
#endif  // _INC_cList_H
