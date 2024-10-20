//! @file cRange.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cRange_H
#define _INC_cRange_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cDebugAssert.h"

namespace Gray {
/// <summary>
/// Simple linearity range from _Lo to _Hi. Similar to cStreamProgressT<>
/// @note assume Normalized Hi>=Lo.
/// POD class should allow static init
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <typename TYPE = int>
struct cRangeT {
    typedef cRangeT<TYPE> THIS_t;

    TYPE _Lo;  /// low range value.
    TYPE _Hi;  /// inclusive high side of range. int size = (hi-lo)+1, float size = hi-lo ?? weird.

    cRangeT() {}  // not init!
    cRangeT(TYPE lo, TYPE hi) : _Lo(lo), _Hi(hi) {}

    inline bool isNormal() const noexcept {
        return _Lo <= _Hi;
    }
    TYPE get_Min() const noexcept {
        return _Lo;
    }
    TYPE get_Max() const noexcept {
        return _Hi;
    }
    TYPE get_Avg() const noexcept {
        return (_Lo + _Hi) / 2;
    }

    /// <summary>
    /// Get value clamped to range.
    /// assume isNormal().
    /// </summary>
    TYPE GetClampValue(TYPE nVal) const noexcept {
        if (nVal < _Lo) return _Lo;
        if (nVal > _Hi) return _Hi;
        return nVal;
    }
    /// <summary>
    /// Is the index in the range? inclusive.
    /// assume isNormal().
    /// </summary>
    /// <param name="nVal"></param>
    /// <returns></returns>
    bool IsInsideI(TYPE nVal) const noexcept {
        return nVal >= _Lo && nVal <= _Hi;
    }
    /// <summary>
    /// Is the index in the range? Non inclusive.
    /// @note if size 0 then this is never true !
    /// assume isNormal().
    /// </summary>
    bool IsInsideX(TYPE nVal) const noexcept {
        return nVal >= _Lo && nVal < _Hi;
    }

    /// <summary>
    /// Get range for inclusive int types.
    /// assume isNormal().
    /// </summary>
    TYPE get_RangeI() const noexcept {
        return (_Hi - _Lo) + 1;  // inclusive. integer
    }
    /// <summary>
    /// Get range (size) for exclusive float types.
    /// assume isNormal().
    /// </summary>
    TYPE get_RangeX() const noexcept {
        return _Hi - _Lo;  // exclusive.
    }
    TYPE get_Size() const noexcept {
        return _Hi - _Lo;  // exclusive.
    }

    /// Get a percent of this range. from 0 to 1 float.
    TYPE GetLinear1(float fOne) const noexcept {
        return CastN(TYPE, _Lo + (fOne * get_RangeI()));
    }

    /// <summary>
    /// get a modulus of the range. IsInsideI.
    /// May not be normalized ?
    /// </summary>
    /// <param name="iVal"></param>
    /// <returns></returns>
    int GetSpinValueI(int iVal) const {
        iVal -= (int)_Lo;
        const int iRange = (int)get_RangeI();
        iVal %= iRange;
        iVal += (iVal < 0) ? CastN(int, _Hi + 1) : CastN(int, _Lo);
        DEBUG_CHECK(IsInsideI(CastN(TYPE, iVal)));
        return iVal;
    }

    // Setters.
    void SetZero() noexcept {
        _Hi = _Lo = 0;
    }
    void put_Min(TYPE iLo) noexcept {
        _Lo = iLo;
    }
    void put_Max(TYPE iHi) noexcept {
        _Hi = iHi;
    }
    void SetRange(TYPE iLo, TYPE iHi) noexcept {
        //! May not be normalized ?
        _Lo = iLo;
        _Hi = iHi;
    }
    void NormalizeRange() noexcept {
        if (!isNormal()) cMem::SwapT(_Lo, _Hi);         
    }

    /// <summary>
    /// Expand the range to include this value. ASSUME isNormal()
    /// </summary>
    void UnionValue(TYPE nVal) noexcept {
        if (nVal < _Lo) _Lo = nVal;
        if (nVal > _Hi) _Hi = nVal;
    }

    /// <summary>
    /// Do 2 ranges overlap ? (assume isNormal()/proper ordered ranges)
    /// </summary>
    /// <param name="x"></param>
    /// <returns></returns>
    bool IsRangeOverlapI(const THIS_t& x) const noexcept {
        if (x._Lo > _Hi) return false;
        if (x._Hi < _Lo) return false;
        return true;
    }
    /// <summary>
    /// assume isNormal()/proper ordered ranges
    /// </summary>
    /// <param name="x"></param>
    void SetUnionRange(const THIS_t& x) noexcept {
        if (x._Hi > _Hi) _Hi = x._Hi;
        if (x._Lo < _Lo) _Lo = x._Lo;
    }
};
}  // namespace Gray
#endif
