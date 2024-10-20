//! @file cStack.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cStack_H
#define _INC_cStack_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "cSpan.h"

namespace Gray {
/// <summary>
/// Create a generic thread/multi process safe (static sized) stack.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
/// <typeparam name="_QTY"></typeparam>
template <ITERATE_t _QTY, class TYPE = BYTE>
struct cStackStatic : public cSpanStatic<_QTY, TYPE> {
    typedef cSpanStatic<_QTY, TYPE> SUPER_t;

    ITERATE_t _nSizeUsed = 0;  /// last . cQueueIndex

    inline cStackStatic() noexcept {
        STATIC_ASSERT(_QTY > 0, cStackStatic);
    }
    inline bool isEmpty() const noexcept {
        return _nSizeUsed == 0;
    }
    inline bool isFull() const noexcept {
        return _nSizeUsed >= _QTY;
    }
    bool IsEqual(const TYPE* pId, ITERATE_t nSize) const noexcept {
        if (nSize != this->_nSizeUsed) return false;
        if (pId == nullptr) return false;
        if (!cMem::IsEqual(pId, this->_Data, nSize)) return false;
        return true;
    }
    cSpanX<TYPE> get_SpanUsed() const {
        return ToSpan(this->_Data, _nSizeUsed);
    }

    void SetZero() {
        _nSizeUsed = 0;
        SUPER_t::SetZero();
    }
    inline TYPE Pop() {
        ASSERT(_nSizeUsed >= 1);
        return this->_Data[--_nSizeUsed];
    }
    inline void Push(TYPE v) {
        ASSERT(_nSizeUsed < _QTY);
        this->_Data[_nSizeUsed++] = v;
    }
};
}  // namespace Gray
#endif
