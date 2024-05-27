//! @file ITextWriter.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_ITextWriter_H
#define _INC_ITextWriter_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "HResult.h"
#include "StrArg.h"
#include "StrT.h"

namespace Gray {
/// <summary>
/// Write a string to a cStreamOutput or StrBuilder.
/// </summary>
struct ITextWriter {
    virtual HRESULT WriteString(const char* pszStr) = 0;
    virtual HRESULT WriteString(const wchar_t* pszStr) {
        return WriteString(StrArg<wchar_t>(pszStr));
    }

    /// <summary>
    /// Write just the chars of the string. NOT nullptr
    /// </summary>
    /// <returns>-lt- 0 = error. else number of chars written</returns>
    template <typename _CH>
    HRESULT VPrintf(const _CH* pszFormat, va_list args) {
        ASSERT_NN(pszFormat);
        _CH szTemp[StrT::k_LEN_Default];
        const StrLen_t iLenRet = StrT::vsprintfN(TOSPAN(szTemp), pszFormat, args);
        UNREFERENCED_PARAMETER(iLenRet);
        return WriteString(szTemp);
    }

    /// <summary>
    /// Write just the chars of the string. NOT nullptr
    /// Does NOT assume include NewLine or automatically add one.
    /// @note Use StrArg(s) for safe "%s" args.
    /// </summary>
    /// <typeparam name="_CH">char or wchar_t</typeparam>
    /// <param name="pszFormat"></param>
    /// <param name=""></param>
    /// <returns>-lt- 0 = error. else number of chars written</returns>
    template <typename _CH>
    HRESULT _cdecl Printf(const _CH* pszFormat, ...) {
        ASSERT_NN(pszFormat);
        va_list vargs;
        va_start(vargs, pszFormat);
        const HRESULT hResLen = VPrintf(pszFormat, vargs);
        va_end(vargs);
        return hResLen;
    }

    /// <summary>
    /// Repeat writing of a char/wchar_t * nCount.
    /// </summary>
    /// <typeparam name="_CH"></typeparam>
    /// <param name="nChar"></param>
    /// <param name="nCount"></param>
    /// <returns>-lt- 0 = error.</returns>
    template <typename _CH>
    HRESULT WriteCharRepeat(_CH nChar, int nCount = 1) {
        ASSERT(nCount >= 0);
        _CH szTmp[2];
        szTmp[0] = nChar;
        szTmp[1] = '\0';
        for (; nCount-- > 0;) {
            const HRESULT hRes = WriteString(szTmp);
            if (FAILED(hRes)) return hRes;
        }
        return S_OK;
    }
};
}
#endif
