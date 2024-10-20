//! @file cNonCopyable.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cNonCopyable_H
#define _INC_cNonCopyable_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray {
#if defined(_MSC_VER) && _MSC_VER <= 1600  // No C++11 features.
                                           // Get rid of C++11 features. e.g. "= delete" and override
#define IS_DELETE
#else
#define IS_DELETE = delete  // to get rid of this normally default supplied method.
#endif

/// <summary>
/// Block C++ usage of default copy constructor. base a class on this with protected type.
/// Similar to use of C++11 delete -> IS_DELETE
/// http://stackoverflow.com/questions/4172722/what-is-the-rule-of-three
/// @note don't use inheritance for templates defined in DLL/SO that might have statics. Use the DECLARE_cNonCopyable instead.
/// also avoid GRAYCORE_LINK for templates.
/// movable may be implemented for parent.
/// </summary>
class GRAYCORE_LINK cNonCopyable {
 protected:
    /// Force the use of Factory creation via protected constructor.
    inline cNonCopyable() noexcept {}

    /// Restrict the copy constructor and assignment operator
    /// __GNUC__ - defaulted and deleted functions only available with -std=c++11 or -std=gnu++11
    /// Make this a macro to avoid linkage inconsistency with use in DLL/SO and templates.
#define DECLARE_cNonCopyable(_TYPE)       \
 private:                                 \
    inline _TYPE(const _TYPE&) IS_DELETE; \
    inline const _TYPE& operator=(const _TYPE&) IS_DELETE;

    DECLARE_cNonCopyable(cNonCopyable);
};
}  // namespace Gray
#endif  // cNonCopyable
