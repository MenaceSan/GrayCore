//! @file cExceptionAssert.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cExceptionAssert_H
#define _INC_cExceptionAssert_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cException.h"

namespace Gray {
/// <summary>
/// Asserts can be seen as exceptions. details from coded ASSERT stuff.
/// </summary>
class GRAYCORE_LINK cExceptionAssert : public cException {
 protected:
    const LOGCHAR_t* const _pExp;
    const cDebugSourceLine _Src;  // DEBUGSOURCELINE __FILE__ __LINE__
 public:
    cExceptionAssert(const LOGCHAR_t* pExp, LOGLVL_t eSeverity, const cDebugSourceLine& src);
    ~cExceptionAssert() THROW_DEF override;
    BOOL GetErrorMessage(StrBuilder<GChar_t>& sb, UINT* pnHelpContext) override;
    static void GRAYCALL Throw(const LOGCHAR_t* pExp, const cDebugSourceLine& src);
};
}  // namespace Gray

#endif
