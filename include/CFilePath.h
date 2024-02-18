//! @file cFilePath.h
//! Manipulate file path names.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cFilePath_H
#define _INC_cFilePath_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "cString.h"

namespace Gray {
typedef cStringT<FILECHAR_t> cStringF;  /// A file name. checks USE_UNICODE_FN. related to cFilePath.

/// <summary>
/// enumerate file system types
/// The file system type dictates what characters are allowed in names and if the names are case sensitive.
/// Similar to cZipFileEntry1::SYSTEM_TYPE
/// </summary>
enum class FILESYS_t {
    _DEFAULT = 0,  /// Modern OS's. >= FILESYS_FAT32
    _FAT,          /// Old DOS 8.3 names. Most restrictive of chars allowed. SYSTEM_MSDOS
    _FAT32,        /// Allow spaces and ~1 equivalent names. long names. SYSTEM_VFAT
    _NTFS,         /// Allow long names, spaces and ~1 equivalent names + extra Attributes. SYSTEM_WINDOWS_NTFS
    _NFS,          /// Linux names are case sensitive. typical for FTP or HTTP mounted files. SYSTEM_UNIX
    _QTY,
};

/// <summary>
/// flags to categorize characters that might be in file paths.
/// What chars are valid for a file name. Group chars into categories and sections for use in a file name.
/// http://en.wikipedia.org/wiki/Filename
/// </summary>
enum FILECHR_MASK_t : BYTE {
    FILECHR_Invalid = 0,    /// not a valid char.

    FILECHR_Device = 0x01,  /// Volume: designators (drive letters) or device ? "a:" or "com1:"
    FILECHR_Dirs = 0x02,    /// FILECHAR_IsDirSep, the directory path to the file. may have relatives ".."

    FILECHR_Name = 0x04,    /// Char is valid as part of a FILESYS_t file name.
    FILECHR_Name2 = 0x08,   /// Char is part of name but not in base set. "spaces, <, >, |, \, and sometimes :, (, ), &, ;, #, as well as wildcards such as ? and *" requires quotes or escaping.
    FILECHR_Name3 = 0x10,   /// Char is part of name but is foreign char. UNICODE.
    FILECHR_Ext = 0x20,     /// File type extension. '.' for ".txt"

    FILECHR_XPath = 0x3E,   /// mask for path but NOT Volume/drive. good for FTP
    FILECHR_All = 0x3F,     /// mask for any (including Volume/drive). but not wildcard.

    FILECHR_Wildcard = 0x40,  /// "?*" = allow wildcard in the name.
};
 
/// <summary>
/// File Name and Path manipulation helper functions.
/// Manipulation of a files name parts such as found in FILECHR_MASK_t
/// similar to .NET System.IO.Path or __linux__ libpath_utils, or std::filesystem::path
/// @note use _MAX_PATH for max length of file path.
/// </summary>
class GRAYCORE_LINK cFilePath : public cStringF {
 public:
    typedef cStringF SUPER_t;

    static const StrLen_t k_MaxLen = _MAX_PATH;  /// Use _MAX_PATH for max length of a file path.
    static const FILECHAR_t k_DirSep1 = '/';     /// preferred for __linux__ NFS but allowed by _WIN32.
    static const FILECHAR_t k_DirSep2 = '\\';    /// preferred for _WIN32 but NOT allowed in __linux__.

#ifdef _WIN32
    static const FILECHAR_t k_DirSep = '\\';    /// preferred for _WIN32. Not allowed in __linux__.
#define FILESTR_DirSep "\\"
#else
    static const FILECHAR_t k_DirSep = '/';     /// preferred for __linux__ NFS but allowed by _WIN32.
#define FILESTR_DirSep "/"
#endif

#ifdef _WIN32
    static const FILECHAR_t k_NamePrefix[5];  /// Prefix required for Long names in _WIN32. From _MAX_PATH to 32k.
#endif

 public:
    cFilePath() noexcept : SUPER_t() {}
    cFilePath(const cStringF& s) noexcept : SUPER_t(s) {}
    cFilePath(const FILECHAR_t* p) noexcept : SUPER_t(p) {}

    /// <summary>
    /// Is this char a dir sep? FAT honors backslash. NTFS honors both chars. NFS uses just the forward slash.
    /// FILECHR_Dirs, like MFC IsDirSep()
    /// </summary>
    /// <param name="ch"></param>
    /// <returns>bool</returns>
    static inline constexpr bool IsCharDirSep(wchar_t ch) noexcept {
        return ch == k_DirSep1 || ch == k_DirSep2;
    }
    static inline constexpr bool IsCharWildcard(wchar_t ch) noexcept {
        //! FILECHR_Wildcard
        return ch == '?' || ch == '*';
    }

    static FILECHR_MASK_t GRAYCALL GetFileCharType(wchar_t ch, FILESYS_t eSys = FILESYS_t::_DEFAULT) NOEXCEPT;
    static bool GRAYCALL IsFileNameValid(const FILECHAR_t* pszName, FILECHR_MASK_t uCharMask = FILECHR_All, FILESYS_t eSys = FILESYS_t::_DEFAULT);
    static bool GRAYCALL IsFileNameExt(const FILECHAR_t* pszFileName, const FILECHAR_t* pszExt) noexcept;
    static bool GRAYCALL HasTitleWildcards(const FILECHAR_t* pszPath);

    static FILECHAR_t* GRAYCALL GetFileNameExt(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, bool bMultiDot = false);
    static StrLen_t GRAYCALL StripFileExt(cSpanX<FILECHAR_t>& fileName, bool bMultiDot = false);
    static cFilePath GRAYCALL ReplaceFileExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* pszExtNew);  // ChangeExtension
    static cFilePath GRAYCALL GetNameExtStar(const FILECHAR_t* pszFilePath);

    /// <summary>
    /// Get file name and ext. (not path or drive) (File title)
    /// using iLen we might back up to the directory under any other directory.
    /// similar to COMMDLG.H GetFileTitleA(const char*, char*, WORD)
    /// </summary>
    static FILECHAR_t* GRAYCALL GetFileName(const FILECHAR_t* pszPath, StrLen_t iLenPath = k_StrLen_UNK);

    /// <summary>
    /// Get the file name title with NO extension and NO path.
    /// </summary>
    static cFilePath GRAYCALL GetFileNameNE(const FILECHAR_t* pszPath, StrLen_t iLenPath = k_StrLen_UNK, bool bMultiDot = false);

    /// <summary>
    /// make a symbolic name from a file name. replace directory separators with _ chSub ATOMCHAR_t
    /// </summary>
    /// <param name="pszOut">Limit to k_LEN_MAX_CSYM</param>
    /// <param name="pszPath">source</param>
    /// <param name="chSub">'_'</param>
    /// <param name="flags">FILECHR_Dirs | FILECHR_Name</param>
    /// <returns>length of the string.</returns>
    static StrLen_t GRAYCALL MakeSymName(ATOMCHAR_t* pszOut, const FILECHAR_t* pszPath, ATOMCHAR_t chSub = '_', BYTE flags = FILECHR_Dirs | FILECHR_Name);

    /// <summary>
    /// Convert the file name into a useful symbolic name. (SYMNAME)
    /// remove the directory and extension name.
    /// </summary>
    /// <returns>cStringA</returns>
    static cStringA GRAYCALL MakeSymNameStr(const FILECHAR_t* pszPath, ATOMCHAR_t chSub = '_', BYTE flags = FILECHR_Dirs | FILECHR_Name);

    static StrLen_t GRAYCALL MakeFullPath2(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);
    static StrLen_t GRAYCALL MakeFullPath(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);
    static cFilePath GRAYCALL MakeFullPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);

    static StrLen_t GRAYCALL MakeProperPath(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFileInp = nullptr, FILECHAR_t chSep = k_DirSep);
    static cFilePath GRAYCALL MakeProperPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);

    static StrLen_t GRAYCALL AddFileDirSep(FILECHAR_t* pszOut, StrLen_t iLen = k_StrLen_UNK, FILECHAR_t chSep = k_DirSep);
    static cFilePath GRAYCALL RemoveFileDirSep(const cStringF& sDir);

    static StrLen_t GRAYCALL CombineFilePathA(cSpanX<FILECHAR_t>& ret, StrLen_t iLen1, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);
    static StrLen_t GRAYCALL CombineFilePath(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszDir, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);  // .NET Combine
    static cFilePath GRAYCALL CombineFilePathX(const FILECHAR_t* pszBase, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);
    static cFilePath _cdecl CombineFilePathF(FILECHAR_t chSep, const FILECHAR_t* pszBase, ...);

    static StrLen_t GRAYCALL ExtractDir(OUT FILECHAR_t* pszPath, StrLen_t iLen = k_StrLen_UNK, bool bTrailingSep = true);
    static StrLen_t GRAYCALL ExtractDirCopy(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFilePathSrc, bool bTrailingSep = true);
    static cFilePath GRAYCALL GetFileDir(const FILECHAR_t* pszFilePath, bool bTrailingSep = true);

    static bool GRAYCALL IsRelativeRoot(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir, StrLen_t iLen = k_StrLen_UNK);
    static bool GRAYCALL IsRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRelativePath);
    static cFilePath GRAYCALL MakeRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir);

    static StrLen_t GRAYCALL GetFilePathDeviceLen(const FILECHAR_t* pszNameRoot);
    static bool GRAYCALL IsFileDeviceRemote(const FILECHAR_t* pszPath);
    static bool GRAYCALL IsFilePathRooted(const FILECHAR_t* pszName);  // .NET IsPathRooted
    static bool GRAYCALL IsFilePathRoot(const FILECHAR_t* pszName);
    static bool GRAYCALL IsFilePathTitle(const FILECHAR_t* pszName);
    static bool GRAYCALL HasFilePathRelatives(const FILECHAR_t* pszName, bool bOrDevices = true);

    static FILECHAR_t* GRAYCALL SkipRelativePrefix(const FILECHAR_t* pszName);
    static FILECHAR_t* GRAYCALL GetFilePathUpDir2(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, int iQtyDirs = 1);
    static cStringF GRAYCALL GetFilePathUpDir1(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, int iQtyDirs = 1);
    static bool GRAYCALL MakeFilePathUpDir(FILECHAR_t* pszName);

    //! @note Path may or may not be case sensitive! ignore trailing dir sep.
    static COMPARE_t GRAYCALL ComparePath(const FILECHAR_t* pszPath1, const FILECHAR_t* pszPath2, StrLen_t iLenMax = _MAX_PATH) noexcept;

#ifdef _WIN32
    static const wchar_t* GRAYCALL MakeFileNameLongW(const FILECHAR_t* pszFilePath);
    static const wchar_t* GRAYCALL GetFileNameLongW(cStringF sFilePath);

    /// <summary>
    /// Convert FileName to long filename. _WIN32.
    /// Add _WIN32 k_NamePrefix if the filename is too long for the system call.
    /// Convert UTF8 to UNICODE if necessary.
    /// </summary>
    /// <param name="pszFilePath"></param>
    /// <returns></returns>
    static const wchar_t* GRAYCALL GetFileNameLongW(const FILECHAR_t* pszFileName);
#endif

    cFilePath MakeFullPath(cStringF sRelPath) const {
        //! Given a relative resource file name. Make the full path to the file.
        if (cFilePath::IsFilePathRooted(sRelPath)) {
            return sRelPath;
        }
        return cFilePath::CombineFilePathX(get_CPtr(), sRelPath);  // relative.
    }
    cFilePath MakeRelativePath(const FILECHAR_t* pszFullPath) const {
        //! Given a full path, get a path relative to this root (if possible) else return full path.
        return cFilePath::MakeRelativePath(pszFullPath, get_CPtr());
    }
};
}  // namespace Gray
#endif  // _INC_cFilePath_H
