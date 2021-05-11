//
//! @file cFilePath.h
//! Manipulate file path names.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cFilePath_H
#define _INC_cFilePath_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cString.h"
#include "FileName.h"

namespace Gray
{
	typedef cStringT<FILECHAR_t> cStringF;	//!< A file name. checks USE_UNICODE_FN. related to cFilePath.

	enum FILESYS_TYPE
	{
		//! @enum Gray::FILESYS_TYPE
		//! The file system type dictates what characters are allowed in names and if the names are case sensitive.
		//! Similar to cZipFileEntry1::SYSTEM_TYPE

		FILESYS_DEFAULT = 0,	//!< Modern OS's. >= FILESYS_FAT32
		FILESYS_FAT,		//!< Old DOS 8.3 names. Most restrictive of chars allowed.
		FILESYS_FAT32,		//!< Allow spaces and ~1 equivalent names. long names.
		FILESYS_NTFS,		//!< Allow long names, spaces and ~1 equivalent names + extra Attributes.
		FILESYS_NFS,		//!< Linux names are case sensitive. typical for FTP or HTTP mounted files.
		FILESYS_QTY,
	};

	enum FILECHR_TYPE_
	{
		//! @enum Gray::FILECHR_TYPE_
		//! What chars are valid for a file name. Group chars into categories and sections for use in a file name.
		//! http://en.wikipedia.org/wiki/Filename

		FILECHR_Invalid = 0,	//!< not a valid char.

		FILECHR_Device = 0x01,	//!< Volume: designators (drive letters) or device ? "a:" or "com1:"
		FILECHR_Dirs = 0x02,	//!< FILECHAR_IsDirSep, the directory path to the file. may have relatives ".."

		FILECHR_Name = 0x04,	//!< Char is valid as part of a FILESYS_TYPE file name.
		FILECHR_Name2 = 0x08,	//!< Char is part of name but not in base set. "spaces, <, >, |, \, and sometimes :, (, ), &, ;, #, as well as wildcards such as ? and *" requires quotes or escaping.
		FILECHR_Name3 = 0x10,	//!< Char is part of name but is foreign char. UNICODE.
		FILECHR_Ext = 0x20,	//!< File type extension. '.' for ".txt"

		FILECHR_XPath = 0x3E, //!< mask for path but NOT Volume/drive. good for FTP
		FILECHR_All = 0x3F,	//!< mask for any (including Volume/drive). but not wildcard.

		FILECHR_Wildcard = 0x40,	//!< "?*" = allow wildcards in the name.
	};
	typedef WORD FILECHR_MASK_t;	//!< Mask of enum FILECHR_TYPE_

	class GRAYCORE_LINK cFilePath : public cStringF
	{
		//! @class Gray::cFilePath
		//! File Name and Path manipulation helper functions.
		//! Manipulation of a files name parts such as found in FILECHR_TYPE_
		//! similar to .NET System.IO.Path
		//! similar to __linux__ libpath_utils
		//! @note use _MAX_PATH for max length of file path.

	public:
		static const StrLen_t k_MaxLen = _MAX_PATH;	//!< Use _MAX_PATH for max length of a file path.
		static const FILECHAR_t k_DirSep1 = '/';	//!< preferred for __linux__ NFS but allowed by _WIN32.
		static const FILECHAR_t k_DirSep2 = '\\';	//!< preferred for _WIN32 but NOT allowed in __linux__.

#ifdef _WIN32
		static const FILECHAR_t k_DirSep = '\\';	//!< preferred for _WIN32. Not allowed in __linux__.
#define FILESTR_DirSep		"\\"
#else
		static const FILECHAR_t k_DirSep = '/';		//!< preferred for __linux__ NFS but allowed by _WIN32.
#define FILESTR_DirSep		"/"
#endif

#ifdef _WIN32
		static const FILECHAR_t k_NamePrefix[5];		//!< Prefix required for Long names in _WIN32. From _MAX_PATH to 32k.
#endif

	public:
		static inline constexpr bool IsCharDirSep(wchar_t ch) noexcept
		{
			//! FAT honors backslash. NTFS honors both chars. NFS uses just the forward slash.
			//! FILECHR_Dirs, like MFC IsDirSep()
			return ch == k_DirSep1 || ch == k_DirSep2;
		}
		static inline constexpr bool IsCharWildcard(wchar_t ch) noexcept
		{
			//! FILECHR_Wildcard
			return  ch == '?' || ch == '*';
		}

		static FILECHR_TYPE_ GRAYCALL GetFileCharType(wchar_t ch, FILESYS_TYPE eSys = FILESYS_DEFAULT);
		static bool GRAYCALL IsFileNameValid(const FILECHAR_t* pszName, FILECHR_MASK_t uCharMask = FILECHR_All, FILESYS_TYPE eSys = FILESYS_DEFAULT);
		static bool GRAYCALL IsFileNameExt(const FILECHAR_t* pszFileName, const FILECHAR_t* pszExt) noexcept;
		static bool GRAYCALL HasTitleWildcards(const FILECHAR_t* pszPath);

		static FILECHAR_t* GRAYCALL GetFileNameExt(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, bool bMultiDot = false);
		static StrLen_t GRAYCALL StripFileExt(FILECHAR_t* pszFile, StrLen_t iLen, bool bMultiDot = false);
		static cStringF GRAYCALL ReplaceFileExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* pszExtNew); // ChangeExtension
		static cStringF GRAYCALL GetNameExtStar(const FILECHAR_t* pszFilePath);

		static FILECHAR_t* GRAYCALL GetFileName(const FILECHAR_t* pszPath, StrLen_t iLenPath = k_StrLen_UNK);
		static cStringF GRAYCALL GetFileNameNE(const FILECHAR_t* pszPath, StrLen_t iLenPath = k_StrLen_UNK, bool bMultiDot = false);

		static StrLen_t GRAYCALL MakeFileSymbolicName(ATOMCHAR_t* pszOut, const FILECHAR_t* pszPath, const ATOMCHAR_t* pszPrefix = nullptr, ATOMCHAR_t chSub = '_', bool bAllowLeadingNumber = false);
		static cStringA GRAYCALL GetFileSymbolicName(const FILECHAR_t* pszPath, const ATOMCHAR_t* pszPrefix = nullptr, ATOMCHAR_t chSub = '_', bool bAllowLeadingNumber = false);

		static StrLen_t GRAYCALL MakeFullPath2(FILECHAR_t* pszFileOut, const FILECHAR_t* pszFileInp, StrLen_t iLenMax, FILECHAR_t chSep = k_DirSep);
		static StrLen_t GRAYCALL MakeFullPath(FILECHAR_t* pszFileOut, const FILECHAR_t* pszFileInp, StrLen_t iLenMax, FILECHAR_t chSep = k_DirSep);
		static cStringF GRAYCALL MakeFullPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);

		static StrLen_t GRAYCALL MakeProperPath(FILECHAR_t* pszFileOut, StrLen_t iLenMax = _MAX_PATH, const FILECHAR_t* pszFileInp = nullptr, FILECHAR_t chSep = k_DirSep);
		static cStringF GRAYCALL MakeProperPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep = k_DirSep);

		static StrLen_t GRAYCALL AddFileDirSep(FILECHAR_t* pszOut, StrLen_t iLen = k_StrLen_UNK, FILECHAR_t chSep = k_DirSep);
		static cStringF GRAYCALL RemoveFileDirSep(cStringF sDir);

		static StrLen_t GRAYCALL CombineFilePathA(FILECHAR_t* pszOut, StrLen_t iLenMax, StrLen_t iLen, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);
		static StrLen_t GRAYCALL CombineFilePath(FILECHAR_t* pszFilePathOut, StrLen_t iLenMax, const FILECHAR_t* pszDir, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep); // .NET Combine
		static cStringF GRAYCALL CombineFilePathX(const FILECHAR_t* pszBase, const FILECHAR_t* pszName, FILECHAR_t chSep = k_DirSep);
		static cStringF _cdecl CombineFilePathF(FILECHAR_t chSep, const FILECHAR_t* pszBase, ...);

		static StrLen_t GRAYCALL ExtractDir(FILECHAR_t* pszPath, StrLen_t iLen = k_StrLen_UNK, bool bTrailingSep = true);
		static StrLen_t GRAYCALL ExtractDirCopy(FILECHAR_t* pszDirPath, StrLen_t iLenMax, const FILECHAR_t* pszFilePathSrc, bool bTrailingSep = true);
		static cStringF GRAYCALL GetFileDir(const FILECHAR_t* pszFilePath, bool bTrailingSep = true);

		static bool GRAYCALL IsRelativeRoot(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir, StrLen_t iLen = k_StrLen_UNK);
		static bool GRAYCALL IsRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRelativePath);
		static cStringF GRAYCALL MakeRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir);

		static StrLen_t GRAYCALL GetFilePathDeviceLen(const FILECHAR_t* pszNameRoot);
		static bool GRAYCALL IsFileDeviceRemote(const FILECHAR_t* pszPath);
		static bool GRAYCALL IsFilePathRooted(const FILECHAR_t* pszName);	// .NET IsPathRooted
		static bool GRAYCALL IsFilePathRoot(const FILECHAR_t* pszName);
		static bool GRAYCALL IsFilePathTitle(const FILECHAR_t* pszName);
		static bool GRAYCALL HasFilePathRelatives(const FILECHAR_t* pszName, bool bOrDevices = true);

		static FILECHAR_t* GRAYCALL SkipRelativePrefix(const FILECHAR_t* pszName);
		static FILECHAR_t* GRAYCALL GetFilePathUpDir2(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, int iQtyDirs = 1);
		static cStringF GRAYCALL GetFilePathUpDir1(const FILECHAR_t* pszName, StrLen_t iLen = k_StrLen_UNK, int iQtyDirs = 1);
		static bool GRAYCALL MakeFilePathUpDir(FILECHAR_t* pszName);

		//! @note Path may or may not be case sensitive!
		static COMPARE_t GRAYCALL ComparePath(const FILECHAR_t* pszPath1, const FILECHAR_t* pszPath2, StrLen_t iLenMax = _MAX_PATH);

#ifdef _WIN32
		static const wchar_t* GRAYCALL MakeFileNameLongW(const FILECHAR_t* pszFilePath);
		static const wchar_t* GRAYCALL GetFileNameLongW(cStringF sFilePath);
		static const wchar_t* GRAYCALL GetFileNameLongW(const FILECHAR_t* pszFileName);
#endif

	};
}
#endif // _INC_cFilePath_H
