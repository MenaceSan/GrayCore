//! @file cArrayString.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArrayString_H
#define _INC_cArrayString_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cArrayRef.h"
#include "cString.h"

namespace Gray {
/// <summary>
/// Non-sorted array of strings.
/// </summary>
/// <typeparam name="_TYPE_CH"></typeparam>
template <typename _TYPE_CH = TCHAR>
struct GRAYCORE_LINK cArrayString : public cArray<cStringT<_TYPE_CH>, const _TYPE_CH*> {
    typedef cStringT<_TYPE_CH> STR_t;
    typedef cArray<cStringT<_TYPE_CH>, const _TYPE_CH*> SUPER_t;
    typedef cArrayString<_TYPE_CH> THIS_t;

    static const ITERATE_t k_MaxDefault = 32;          /// default max for AddUniqueMax.
    static const ITERATE_t k_MaxElements = 64 * 1024;  /// Max elements. reasonable arbitrary limit.

    cArrayString() noexcept {}
    cArrayString(const _TYPE_CH** ppStr, ITERATE_t iCount) {
        SetCPtrs(ppStr, iCount);
    }
    explicit cArrayString(const cArrayString& a) {
        this->SetCopy(a);
    }
    cArrayString(THIS_t&& ref) noexcept : SUPER_t(ref) {
        //! move constructor.
    }

    inline ~cArrayString() {}

    ITERATE_t FindCmpI(const _TYPE_CH* pszFind) const {
        //! find the (whole) string in the unsorted array of strings. Case Ignored.
        const ITERATE_t iQty = this->GetSize();
        for (ITERATE_t i = 0; i < iQty; i++) {
            if (!StrT::CmpI(this->GetAt(i).get_CPtr(), pszFind)) return i;
        }
        return k_ITERATE_BAD;
    }

    /// <summary>
    /// find inside pszSearch a partial string match of any of the unsorted array of strings. Case Ignored.
    /// </summary>
    ITERATE_t FindStrIR(const _TYPE_CH* pszSearch) const {
        ITERATE_t iQty = this->GetSize();
        for (ITERATE_t i = 0; i < iQty; i++) {
            if (StrT::FindStrI(pszSearch, this->GetAt(i).get_CPtr()) != nullptr) return i;
        }
        return k_ITERATE_BAD;
    }

    /// <summary>
    /// Get string element from the array.
    /// </summary>
    /// <param name="i"></param>
    /// <returns>"" = if index is out of range.</returns>
    STR_t GetAtCheck(ITERATE_t i) const {
        if (!SUPER_t::IsValidIndex(i)) return cStrConst::k_Empty.GetT<_TYPE_CH>();  // STR_t("")
        return SUPER_t::GetAt(i);
    }

    /// <summary>
    /// Set this list from parsing a comma (or other) separated list of strings.
    /// </summary>
    /// <param name="pszStr"></param>
    /// <param name="chSep"></param>
    /// <returns></returns>
    ITERATE_t SetStrSep(const _TYPE_CH* pszStr, _TYPE_CH chSep = ',') {
        _TYPE_CH szSep[2];
        szSep[0] = chSep;
        szSep[1] = '\0';
        _TYPE_CH szTmp[StrT::k_LEN_Default];
        _TYPE_CH* aCmds[k_ARG_ARRAY_MAX];
        const ITERATE_t iStrings = StrT::ParseArrayTmp(TOSPAN(szTmp), pszStr, TOSPAN(aCmds), szSep);
        SetCPtrs((const _TYPE_CH**)aCmds, iStrings);
        return iStrings;
    }

    /// <summary>
    /// Get the list as a CSV string.
    /// </summary>
    /// <param name="chSep"></param>
    /// <param name="iMax"></param>
    /// <returns>the whole array as a single comma separated string.</returns>
    STR_t GetStrSep(_TYPE_CH chSep = ',', ITERATE_t iMax = 0x7FFF) const {
        if (iMax > this->GetSize()) iMax = this->GetSize();
        STR_t sRet;
        for (ITERATE_t i = 0; i < iMax; i++) {
            sRet += SUPER_t::GetAt(i);
            sRet += chSep;
        }
        return sRet;
    }

    /// <summary>
    /// Set array of strings.
    /// </summary>
    void SetCPtrs(const _TYPE_CH** ppStr, ITERATE_t iCount) {
        this->RemoveAll();
        ASSERT(iCount < k_MaxElements);  // reasonable max.
        for (ITERATE_t i = 0; i < iCount; i++) {
            this->Add(ppStr[i]);
        }
    }
    void SetStrings(const cStringT<_TYPE_CH>* ppStr, ITERATE_t iCount) {
        this->RemoveAll();
        ASSERT(iCount < k_MaxElements);  // reasonable max.
        for (ITERATE_t i = 0; i < iCount; i++) {
            this->Add(ppStr[i]);
        }
    }
    void SetStrings(const THIS_t& a) {
        SetStrings(a.get_DataConst(), a.GetSize());
    }

    ITERATE_t _cdecl AddFormat(const _TYPE_CH* pszFormat, ...) {
        cStringT<_TYPE_CH> sTmp;
        va_list vargs;
        va_start(vargs, pszFormat);
        sTmp.FormatV(pszFormat, vargs);
        va_end(vargs);
        return this->Add(sTmp);
    }
   
    /// <summary>
    /// Add a non dupe to the list end. if dupe then return index of match.
    /// Enforce iMax qty by delete head.
    /// </summary>
    ITERATE_t AddUniqueMax(const _TYPE_CH* pszStr, ITERATE_t iMax = k_MaxDefault) {
        if (StrT::IsNullOrEmpty(pszStr) || iMax < 1) return k_ITERATE_BAD;
        ITERATE_t iQty = this->GetSize();
        for (ITERATE_t i = 0; i < iQty; i++) {
            if (!StrT::CmpI(this->GetAt(i).get_CPtr(), pszStr))  { // dupe
                return i;
            }
        }
        while (iQty >= iMax) {
            this->RemoveAt(0);  // roll off extras from head.
            iQty--;
        }
        this->Add(pszStr);
        return iQty;
    }
};

typedef cArrayString<char> cArrayStringA;
typedef cArrayString<wchar_t> cArrayStringW;
}  // namespace Gray
#endif  // _INC_cArrayString_H
