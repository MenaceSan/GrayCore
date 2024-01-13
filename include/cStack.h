//
//! @file cStack.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

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
class cStackStatic : public cSpanStatic<_QTY, TYPE> {
 public:
    ITERATE_t m_nSizeUsed;  /// last . cQueueIndex

 public:
    inline cStackStatic() noexcept : m_nSizeUsed(0) {
        STATIC_ASSERT(_QTY > 0, cStackStatic);
    }
    inline bool isEmpty() const noexcept {
        return m_nSizeUsed == 0;
    }
    inline bool isFull() const noexcept {
        return m_nSizeUsed >= _QTY;
    }
    inline TYPE Pop() {
        ASSERT(m_nSizeUsed >= 1);
        return m_Data[--m_nSizeUsed];
    }
    inline void Push(TYPE v) {
        ASSERT(m_nSizeUsed < _QTY);
        m_Data[m_nSizeUsed++] = v;
    }
};
}  // namespace Gray

#endif
