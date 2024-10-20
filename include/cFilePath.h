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
enum class FILESYS_t : BYTE {
    _DEFAULT = 0,  /// Modern OS's. -gte- FILESYS_FAT32
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
    FILECHR_Invalid = 0,  /// not a valid char.

    FILECHR_Device = 0x01,  /// Volume: designators (drive letters) or device ? "a:" or "com1:"
    FILECHR_Dirs = 0x02,    /// FILECHAR_IsDirSep, the directory path to the file. may have relatives ".."

    FILECHR_Name = 0x04,   /// Char is valid as part of a FILESYS_t file name.
    FILECHR_Name2 = 0x08,  /// Char is part of name but not in base set. "spaces, <, >, |, \, and sometimes :, (, ), &, ;, #, as well as wildcards such as ? and *" requires quotes or escaping.
    FILECHR_Name3 = 0x10,  /// Char is part of name but is foreign char. UNICODE.
    FILECHR_Ext = 0x20,    /// File type extension. '.' for ".txt"

    FILECHR_XPath = 0x3E,  /// mask for path but NOT Volume/drive. good for FTP
    FILECHR_All = 0x3F,    /// mask for any (including Volume/drive). but not wildcard.

    FILECHR_Wildcard = 0x40,  /// "?*" = allow wildcard in the name. IsRegEx
};

/// <summary>
/// File Name and Path manipulation helper functions.
/// Manipulation of a files name parts such as found in FILECHR_MASK_t
/// similar to .NET System.IO.Path or __linux__ libpath_utils, or std::filesystem::path
/// </summary>
class GRAYCORE_LINK cFilePath : public cStringF {
 public:
    typedef cStringF SUPER_t;
    static const char k_DirSep1 = '/';   /// preferred for __linux__ NFS but allowed by _WIN32.
    static const char k_DirSep2 = '\\';  /// preferred for _WIN32 but NOT allowed in __linux__.
    static const char k_szBadChars[];    // chars NEVER allowed.
    static const char k_szAllowedDOS[];  // old DOS names allowed.

#ifdef _WIN32
#ifndef _MAX_PATH
#define _MAX_PATH 260  // __GNUC__ can leave this out if __STRICT_ANSI__
#endif
    static const StrLen_t k_MaxLen = _MAX_PATH;  /// for max length of a file path. 260.
    static const FILECHAR_t k_DirSep = '\\';     /// preferred for _WIN32. Not allowed in __linux__.
#define FILESTR_DirSep "\\"
#else  // __linux__
    static const StrLen_t k_MaxLen = PATH_MAX;  // #include 'limits.h' or MAX_PATH in windef.h. (Put this in Filepath.h ?) FILENAME_MAX ?
    static const FILECHAR_t k_DirSep = '/';     /// preferred for __linux__ NFS but allowed by _WIN32.
#define FILESTR_DirSep "/"
#endif

#ifdef _WIN32
    static const FILECHAR_t k_NamePrefix[5];  /// Prefix required for Long names in _WIN32. From cFilePath::k_MaxLen to 32k.
#endif

 public:
    cFilePath() noexcept : SUPER_t() {}
    cFilePath(const cStringF& s) noexcept : SUPER_t(s) {}
    cFilePath(const cSpan<FILECHAR_t>& src) : SUPER_t(src) {}
    cFilePath(const FILECHAR_t* p) : SUPER_t(p) {}

    /// <summary>
    /// Is this char a dir sep? FAT honors backslash. NTFS honors both chars. NFS uses just the forward slash.
    /// FILECHR_Dirs, like MFC IsDirSep()
    /// </summary>
    /// <param name="ch"></param>
    /// <returns>bool</returns>
    static inline constexpr bool IsCharDirSep(wchar_t ch) noexcept {
        return ch == k_DirSep1 || ch == k_DirSep2;
    }

    /// <summary>
    /// Is the char valid for a filename on FILECHR_MASK_t?
    /// The valid characters for a filename in DOS manual (DOS 5: page 72)
    /// Known Valid:
    ///	 A..Z 0..9 and k_pszAllowedDOS = "_ ^ $ ~ ! # % & - {} () @ ' `"
    /// Known Unvalid:
    ///	 "<>:"|" and "/\?*"
    /// Unknown: (I've seen used but should not be valid?)
    ///  []
    ///  other chars from 128 to 255 may be valid but we may want to filter FILECHR_Name3
    /// http://msdn.microsoft.com/en-us/library/aa365247%28VS.85%29.aspx
    /// http://en.wikipedia.org/wiki/Filename
    /// </summary>
    static FILECHR_MASK_t GRAYCALL GetFileCharType(wchar_t ch, FILESYS_t eSys = FILESYS_t::_DEFAULT) NOEXCEPT;

    /// <summary>
    /// Is this a valid file name? Maybe UTF8.
    /// Do not end a file or directory name with a space or a period.
    /// @arg eSys = FILESYS_t::_FAT = enforce the DOS 8.3 rules.
    /// @note CANNOT have names the same as system devices. e.g. CLOCK$ CON PRN AUX NUL COM# LPT#. Check for those here ?
    /// </summary>
    static bool GRAYCALL IsFileNameValid(const FILECHAR_t* pszName, FILECHR_MASK_t uCharMask = FILECHR_All, FILESYS_t eSys = FILESYS_t::_DEFAULT);

    static bool GRAYCALL IsFileNameExt(const cSpan<FILECHAR_t>& fileName, const cSpan<FILECHAR_t>& ext) noexcept;
    static bool GRAYCALL HasTitleWildcards(const cSpan<FILECHAR_t>& path);

    static FILECHAR_t* GRAYCALL GetFileNameExt(const cSpan<FILECHAR_t>& name, bool bMultiDot = false);

    /// <summary>
    /// strip the ext off the file name (or path).
    /// </summary>
    /// <param name="ret">the known string length of the file name.</param>
    /// <param name="bMultiDot"></param>
    /// <returns>new length of the string.</returns>
    static StrLen_t GRAYCALL StripFileExt(cSpanX<FILECHAR_t> ret, bool bMultiDot = false);

    static cFilePath GRAYCALL ReplaceFileExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* pszExtNew);  // ChangeExtension
    static cFilePath GRAYCALL GetNameExtStar(const cSpan<FILECHAR_t>& path);

    /// <summary>
    /// Get file name and ext. (not path or drive) (File title)
    /// using iLen we might back up to the directory under any other directory.
    /// similar to COMMDLG.H GetFileTitleA(const char*, char*, WORD)
    /// </summary>
    static StrLen_t GRAYCALL GetFileNameI(const cSpan<FILECHAR_t>& path);
    static FILECHAR_t* GRAYCALL GetFileName(const cSpan<FILECHAR_t>& path);

    /// <summary>
    /// Get the file name title with NO extension and NO path.
    /// </summary>
    static cFilePath GRAYCALL GetFileNameNE(const cSpan<FILECHAR_t>& path, bool bMultiDot = false);

    /// <summary>
    /// make a symbolic name from a file name. replace directory separators with _ chSub ATOMCHAR_t
    /// </summary>
    /// <param name="symName">Limit to k_LEN_MAX_CSYM</param>
    /// <param name="pszPath">source</param>
    /// <param name="chSub">'_'</param>
    /// <param name="flags">FILECHR_Dirs | FILECHR_Name</param>
    /// <returns>length of the string.</returns>
    static StrLen_t GRAYCALL MakeSymName(cSpan<ATOMCHAR_t> symName, const FILECHAR_t* pszPath, ATOMCHAR_t chSub = '_', BYTE flags = FILECHR_Dirs | FILECHR_Name);

    /// <summary>
    /// Convert the file name into a useful symbolic name. (SYMNAME)
    /// remove the directory and extension name.
    /// </summary>
    /// <returns>cStringA</returns>
    static cStringA GRAYCALL MakeSymNameStr(const FILECHAR_t* pszPath, ATOMCHAR_t chSub = '_', BYTE flags = FILECHR_Dirs | FILECHR_Name);

    static StrLen_t GRAYCALL MakeFullPath2(cSpanX<FILECHAR_t> ret, const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);
    static StrLen_t GRAYCALL MakeFullPath(cSpanX<FILECHAR_t> ret, const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);

    static cFilePath GRAYCALL MakeFullPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);

    static StrLen_t GRAYCALL MakeProperPath(cSpanX<FILECHAR_t> ret, const FILECHAR_t* pszFileInp = nullptr, FILECHAR_t chSep = k_DirSep);
    static cFilePath GRAYCALL MakeProperPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);

    /// <summary>
    /// Add the k_DirSep to the end to make this a directory.
    /// ASSUME pszOut -gte- cFilePath::k_MaxLen
    /// Might be LINUX
    /// </summary>
    /// <returns>Length</returns>
    static StrLen_t GRAYCALL AddFileDirSep(cSpanX<FILECHAR_t> ret, StrLen_t iLenZ, FILECHAR_t chSep = k_DirSep);

    static cFilePath GRAYCALL RemoveFileDirSep(const cStringF& sDir);

    /// <summary>
    /// Append file/subdir pszName to existing pszFilePath path.
    /// </summary>
    /// <param name="chSep">k_DirSep default.</param>
    /// <returns>New total Length</returns>
    static StrLen_t GRAYCALL CombineFilePathA(cSpanX<FILECHAR_t> ret, StrLen_t iLenZ, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);
    static StrLen_t GRAYCALL CombineFilePath(cSpanX<FILECHAR_t> ret, const FILECHAR_t* pszDir, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);  // .NET Combine

    /// <summary>
    /// Merge/Append path and file name. MakeProperPath.
    /// Similar to Shell PathAppend(), or .NET System.IO.Path.Combine
    /// </summary>
    /// <param name="pszBase"></param>
    /// <param name="pszName"></param>
    /// <param name="chSep">k_DirSep default.</param>
    /// <returns></returns>
    static cFilePath GRAYCALL CombineFilePathX(const FILECHAR_t* pszBase, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);
    static cFilePath _cdecl CombineFilePathF(FILECHAR_t chSep, const FILECHAR_t* pszBase, ...);

    /// <summary>
    /// Remove the file name from this and just leave the path. opposite of GetFileName().
    /// </summary>
    /// <param name="ret">cSpanX</param>
    /// <param name="bTrailingSep">leave the trailing forward slash '/'</param>
    /// <returns>length of the new string.</returns>
    static StrLen_t GRAYCALL ExtractDir(OUT cSpanX<FILECHAR_t> ret, bool bTrailingSep = true);
    static StrLen_t GRAYCALL ExtractDirCopy(OUT cSpanX<FILECHAR_t> ret, const FILECHAR_t* pszFilePathSrc, bool bTrailingSep = true);

    /// <summary>
    /// Extract the directory from a file path. include the trailing / k_DirSep if bTrailingSep is set.
    /// </summary>
    /// <param name="pszFilePath"></param>
    /// <param name="bTrailingSep">leave/add the trailing slash</param>
    static cFilePath GRAYCALL GetFileDir(const FILECHAR_t* pszFilePath, bool bTrailingSep = true);

    static bool GRAYCALL IsRelativeRoot(const FILECHAR_t* pszFullPath, const cSpan<FILECHAR_t>& root);
    static bool GRAYCALL IsRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRelativePath);
    static cFilePath GRAYCALL MakeRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir);

    /// <summary>
    /// Skip the device/drive info at the start of a path. does it have a device name in it ? e.g. "COM1:" or "C:"
    /// "file://", "http://", "c://"
    /// </summary>
    /// <param name="pszNameRooted"></param>
    /// <returns>0=name is NOT rooted.</returns>
    static StrLen_t GRAYCALL GetFilePathDeviceLen(const FILECHAR_t* pszNameRooted);

    /// <summary>
    /// Is the file based on some remote device/service? e.g. HTTP, HTTPS, FTP, RTP, RTMP etc.
    /// GetFilePathDeviceLen
    /// like NETSERVICE_t
    /// </summary>
    static bool GRAYCALL IsFileDeviceRemote(const FILECHAR_t* pszPath);

    static bool GRAYCALL IsFilePathRooted(const FILECHAR_t* pszName);  // .NET IsPathRooted
    static bool GRAYCALL IsFilePathRoot(const FILECHAR_t* pszName);

    /// <summary>
    /// Does this NOT have path/dir indicators?
    /// </summary>
    static bool GRAYCALL IsFilePathTitle(const FILECHAR_t* pszName);

    /// <summary>
    /// Does the file have any relative components. like ..
    /// Get rid of them with MakeProperPath()
    /// </summary>
    static bool GRAYCALL HasFilePathRelatives(const FILECHAR_t* pszName, bool bOrDevices = true);

    static FILECHAR_t* GRAYCALL SkipRelativePrefix(const FILECHAR_t* pszName);
    static FILECHAR_t* GRAYCALL GetFilePathUpDir2(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, int iQtyDirs = 1);
    static cStringF GRAYCALL GetFilePathUpDir1(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, int iQtyDirs = 1);
    static bool GRAYCALL MakeFilePathUpDir(FILECHAR_t* pszName);

    //! @note Path may or may not be case sensitive! ignore trailing dir sep.
    static COMPARE_t GRAYCALL ComparePath(const FILECHAR_t* pszPath1, const FILECHAR_t* pszPath2, StrLen_t iLenMax = cFilePath::k_MaxLen) noexcept;

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

    /// <summary>
    /// Given a relative resource file name. Make the full path to the file.
    /// </summary>
    cFilePath MakeFullPath(cStringF sRelPath) const {
        if (cFilePath::IsFilePathRooted(sRelPath)) return sRelPath;
        return cFilePath::CombineFilePathX(get_CPtr(), sRelPath);  // relative.
    }
    /// <summary>
    /// Given a full path, get a path relative to this root (if possible) else return full path.
    /// </summary>
    cFilePath MakeRelativePath(const FILECHAR_t* pszFullPath) const {
        return cFilePath::MakeRelativePath(pszFullPath, get_CPtr());
    }
};
}  // namespace Gray
#endif  // _INC_cFilePath_H
