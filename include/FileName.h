//! @file FileName.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_FileName_H
#define _INC_FileName_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

namespace Gray {
// File names might be _UNICODE wchar_t or UTF8 characters.
#if USE_UNICODE_FN
typedef wchar_t FILECHAR_t;  /// a _UNICODE char in a file name. like TCHAR. default for _WIN32 OS file names.
#define _FN(x) __TOW(x)      /// like _T(x) macro for static text file names.
#define _FNF(c) c##W         /// _WIN32 name has a A or W for UTF8 or UNICODE
#define _FNFW(c) c##W        /// _WIN32 name has a W for UNICODE but not UTF8
#else
typedef char FILECHAR_t;  /// a UTF8 char in a file name. like TCHAR
#define _FN(x) __TOA(x)  /// like _T(x) macro for static text file names.
#define _FNF(c) c##A     /// _WIN32 name has a A or W for UTF8 or UNICODE
#define _FNFW(c) c       /// _WIN32 name has a W for UNICODE but not UTF8
#endif

/// <summary>
/// Operations on files.
/// Same values as WIN32 "shellapi.h" FO_MOVE, FO_COPY, FO_DELETE, FO_RENAME for cFileDirDlg SHFileOperation.
/// for use with FILEOPF_t/FILEOP_FLAGS ?
/// </summary>
enum class FILEOP_t {
    _Move = 1,    /// AKA FO_MOVE (shellapi.h)
    _Copy = 2,    /// AKA FO_COPY (shellapi.h)
    _Delete = 3,  // FO_DELETE
    _Rename = 4,  /// Similar to FILEOP_t::_Move, FO_RENAME
};

/// <summary>
/// DWORD of flags to control directory listing.
/// Extend FILEOP_FLAGS (FOF_*) from include "shellapi.h" (in global namespace) FOF_FILESONLY
/// </summary>
enum FILEOPF_t : DWORD {
    _None = 0,
#ifdef __linux__
    FOF_ALLOWUNDO = 0x0040,          /// 0x0040 in WIN32 shellapi.h
    FOF_FILESONLY = 0x0080,          /// 0x0080 in WIN32 shellapi.h
    FOF_RENAMEONCOLLISION = 0x0100,  /// 8 in WIN32 (FOF_SIMPLEPROGRESS)
    FOF_NOERRORUI = 0x400,           /// 0x0400  in WIN32 shellapi.h
#endif
    // NON WIN32 standard.
    FOF_X_FollowLinks = 0x10000,
    FOF_X_WantDots = 0x20000,       /// Why would anyone want the .. ?
};
}  // namespace Gray
#endif
