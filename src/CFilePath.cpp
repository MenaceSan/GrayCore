//
//! @file CFilePath.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "CFilePath.h"
#include "CLogMgr.h"
#include "CAppState.h"
#include "CFileDir.h"
#include "StrChar.h"
#include "StrConst.h"

#ifdef __linux__
//#include <ctype.h>
#elif defined(_WIN32) && ! defined(UNDER_CE)
#include <direct.h>
#endif

namespace Gray
{

#ifdef _WIN32
	const FILECHAR_t CFilePath::k_NamePrefix[5] = _FN("\\\\?\\");	//!< pre-pend "\\?\" to the path to extend this limit to 32,767 on NTFS.
#endif

	FILECHR_TYPE_ GRAYCALL CFilePath::GetFileCharType(wchar_t ch, FILESYS_TYPE eSys) // static
	{
		//! Is the char valid for a filename on FILECHR_TYPE_?
		//! The valid characters for a filename in DOS manual (DOS 5: page 72)
		//! Known Valid:
		//!	 A..Z 0..9 and k_pszAllowedDOS = "_ ^ $ ~ ! # % & - {} () @ ' `"
		//! Known Unvalid:
		//!	 "<>:"|" and "/\?*"
		//! Unknown: (I've seen used but should not be valid?)
		//!  []
		//!  other chars from 128 to 255 may be valid but we may want to filter FILECHR_Name3
		//! http://msdn.microsoft.com/en-us/library/aa365247%28VS.85%29.aspx
		//! http://en.wikipedia.org/wiki/Filename

		static const char* k_pszBadChars = "<>\"|"; // chars NEVER allowed.
		static const char* k_pszAllowedDOS = "^$~!#%&-{}()@'`"; // old DOS names allowed.

		if (ch < 255)
		{
			if (ch == '\0')	// never allowed chars.
				return FILECHR_Invalid;
			if (ch < ' ')
				return FILECHR_Invalid;	// UTF8 UNICODE?
			if (StrChar::IsAlNum(ch))	// ASCII letter or number is always ok.
				return FILECHR_Name;
			if (IsCharWildcard(ch))
				return FILECHR_Wildcard;
#if defined(USE_UNICODE_FN)
			if (ch <= 255)	// NOT UNICODE char 
#endif
			{
				if (StrT::HasChar(k_pszBadChars, (char)ch))	// never allowed as name
					return FILECHR_Invalid;
				if (StrT::HasChar(k_pszAllowedDOS, (char)ch))	// old DOS names allowed chars.
					return FILECHR_Name2;	// May need escape container.
			}
		}

		switch (ch)
		{
		case '_':	// always allowed non escaped.
			return FILECHR_Name;
		case '/':		// IsCharDirSep(ch)
			if (eSys == FILESYS_FAT)
				return FILECHR_Invalid;
			return FILECHR_Dirs;
		case '\\':
			if (eSys == FILESYS_NFS)
				return FILECHR_Name;
			return FILECHR_Dirs;
		case ':':
			return FILECHR_Device;
		case '.':
			// NFS first char uses this to hide files.
			return FILECHR_Ext;
		case '+':	// reserved for FAT32
		case '[':	// reserved for FAT32
		case ']':	// reserved for FAT32
			if (eSys == FILESYS_NFS || eSys == FILESYS_NTFS)
				return FILECHR_Name2;
			return FILECHR_Invalid;
		case ' ':
		case ',':
			if (eSys == FILESYS_FAT)
				return FILECHR_Invalid;
			return FILECHR_Name2;
		case 0xA5:	// Yen symbol. Its a FILECHR_Dirs symbol in Japan ?? http://msdn.microsoft.com/en-us/library/windows/desktop/dd317748%28v=vs.85%29.aspx
			return FILECHR_Invalid;
		}

		if (eSys == FILESYS_FAT)
			return FILECHR_Invalid;		// everything else is bad for old DOS FAT.

		return FILECHR_Name3;	// some other sort of char like foreign char. but allowed.
	}

	bool GRAYCALL CFilePath::IsFileNameValid(const FILECHAR_t* pszName, FILECHR_MASK_t uCharMask, FILESYS_TYPE eSys) // static
	{
		//! Is this a valid file name? Maybe UTF8.
		//! Do not end a file or directory name with a space or a period.
		//! @arg eSys = FILESYS_FAT = enforce the DOS 8.3 rules.
		//! @note CANNOT have names the same as system devices. e.g. CLOCK$ CON PRN AUX NUL COM# LPT#. Check for those here ?

		if (pszName == nullptr)
			return false;

		StrLen_t i = 0;
		for (; pszName[i] != '\0'; i++)
		{
			FILECHAR_t ch = pszName[i];
			if (!(uCharMask & GetFileCharType(ch, eSys)))
				return false;
		}
		if (i > 0)
		{
			// Do not end a file or directory name with a space or a period
			FILECHAR_t ch = pszName[i - 1];
			if (ch == '.' || ch == ' ')
				return false;
		}
		return true;
	}

	bool GRAYCALL CFilePath::IsFilePathTitle(const FILECHAR_t* pszName) // static
	{
		//! Does this NOT have path/dir indicators?
		if (pszName == nullptr)
			return false;
		for (StrLen_t i = 0;; i++, pszName++)
		{
			if (i > _MAX_PATH - 1)
				break;	// weird.
			FILECHAR_t ch = pszName[0];
			if (ch == '\0')
				return true;
			if (IsCharDirSep(ch))	// this sort of access is not allowed.
				break;
			if (ch == ':')	// this sort of access is not allowed.
				break;
		}
		return false;;
	}

	bool GRAYCALL CFilePath::HasFilePathRelatives(const FILECHAR_t* pszName, bool bOrDevices) // static
	{
		//! Does the file have any relative components. like ..
		//! Get rid of them with MakeProperPath()

		if (pszName == nullptr)
			return false;
		for (StrLen_t i = 0;; i++, pszName++)
		{
			if (i > _MAX_PATH - 1)
				break;	// weird.
			FILECHAR_t ch = pszName[0];
			if (ch == '\0')
				return false;
			if (ch == '.' && pszName[1] == '.')	// has relative path ../ of some sort. ./ does not count ?
				break;
			if (bOrDevices)
			{
				if (IsCharDirSep(ch) && IsCharDirSep(pszName[1]))	// this sort of access is not allowed.
					break;
				if (ch == ':')	// this sort of access is not allowed.
					break;
			}
		}
		return true;
	}

	StrLen_t GRAYCALL CFilePath::GetFilePathDeviceLen(const FILECHAR_t* pszName) // static
	{
		//! Skip the device info at the start of the path.
		//! have a device name in it ? e.g. "COM1:" or "C:"
		//! "file://", "http://"

		if (pszName == nullptr)
			return 0;

#ifdef _WIN32
		// Special _WIN32 format?
		if (!StrT::CmpI(pszName, _FN("\\Device\\")))
		{
			// TODO: Handle \\Device\\style ;
		}
#endif

		for (StrLen_t i = 0;; i++)
		{
			FILECHAR_t ch = pszName[i];
			if (ch == ':')	// end of device name.
			{
				if (i == 0)
					break;
				return i + 1;	// e.g. "c:" = 2
			}
			if (!StrChar::IsAlpha(ch))	// not valid device name?
				break;
		}
		return 0;
	}

	bool GRAYCALL CFilePath::IsFileDeviceRemote(const FILECHAR_t* pszPath) // static
	{
		//! Is the file based on some remote device/service? e.g. HTTP, HTTPS, FTP, RTP, RTMP etc.
		//! GetFilePathDeviceLen
		//! like NETSERVICE_TYPE

		if (!StrT::CmpIN(pszPath, _FN("file:"), 5))	// local file system.
			return false;
		StrLen_t nLenDev = GetFilePathDeviceLen(pszPath);
		if (nLenDev <= 2)	// just a local drive letter. Rooted.
			return false;
		return true;	// must be a remote protocol.
	}

	bool GRAYCALL CFilePath::IsFilePathRooted(const FILECHAR_t* pszName) // static
	{
		//! Is the file path absolute ? (not relative path to current directory for process)
		//! Based on drive, device or root ? not have .. in it.
		//! If relative path then use MakeFullPathX() to get full rooted path.
		//! e.g. "COM1:" is true!
		//! "C:\" is true.

		if (StrT::IsNullOrEmpty(pszName))
			return false;
		// starts with '\\'
		StrLen_t iLen = GetFilePathDeviceLen(pszName);
		if (iLen > 0 && pszName[iLen] == '\0')
			return true;
		if (IsCharDirSep(pszName[iLen]))
			return true;
		return false;
	}

	bool GRAYCALL CFilePath::IsFilePathRoot(const FILECHAR_t* pszName) // static
	{
		//! is this the root of a device?
		//! @note Includes the DOT !
		//! e.g. "C:\" is true
		if (StrT::IsNullOrEmpty(pszName))
			return false;
		StrLen_t iLen = GetFilePathDeviceLen(pszName);
		if (pszName[iLen] == '\0')
			return false;
		// end with '\\'
		if (IsCharDirSep(pszName[iLen]) && pszName[iLen + 1] == '\0')
			return true;
		return false;
	}

	FILECHAR_t* GRAYCALL CFilePath::GetFileNameExt(const FILECHAR_t* pszName, StrLen_t iLen, bool bMultiDot)	// static
	{
		//! Get a pointer to the extension of the file.
		//! @note Includes the DOT !
		//! @arg
		//!	iLen = the known string length of the file name. -1 = k_StrLen_UNK = find '\0'
		//!	bMultiDot = ".xt.sdf.fff" is considered a single extension.
		//! @return
		//!  The pointer to the extension in this file name. Includes the DOT.
		//!  nullptr = has no extension.

		if (pszName == nullptr)
			return nullptr;
		if (iLen <= k_StrLen_UNK)
		{
			iLen = StrT::Len(pszName);
		}
		FILECHAR_t* pszExt = nullptr;	// has no valid ext.
		while (iLen > 0)
		{
			iLen--;
			FILECHAR_t ch = pszName[iLen];
			if (ch == '.')
			{
				pszExt = (FILECHAR_t*)(pszName + iLen);
				if (bMultiDot)
					continue;
				break;
			}
			// FILECHR_Device|FILECHR_Dirs
			if (IsCharDirSep(ch) || ch == ':')
				break;
			if (!(GetFileCharType(ch) & FILECHR_Name))	// valid ext name character?
				break;
		}
		return pszExt;
	}

	StrLen_t GRAYCALL CFilePath::StripFileExt(FILECHAR_t* pszFile, StrLen_t iLen, bool bMultiDot) // static
	{
		//! strip the ext off the file name (or path).
		//! @arg
		//!	iLen = the known string length of the file name.
		//! @return
		//!	new length of the string.
		if (pszFile == nullptr)
			return 0;
		if (iLen <= k_StrLen_UNK)
		{
			iLen = StrT::Len(pszFile);
		}
		else
		{
			pszFile[iLen] = '\0';
		}
		FILECHAR_t* pszExt = GetFileNameExt(pszFile, iLen, bMultiDot);
		if (StrT::IsNullOrEmpty(pszExt))
		{
			return iLen;
		}
		*pszExt = '\0';
		return StrT::Diff(pszExt, pszFile);
	}

	CStringF GRAYCALL CFilePath::ReplaceFileExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* pszExtNew)
	{
		//! Replace the existing Ext with this new one. If it didn't have one then just add this.
		//! @arg pszExtNew = ".EXT" (with dot)

		if (pszFilePath == nullptr)
			return "";

		FILECHAR_t szTemp[_MAX_PATH];
		StrLen_t iLen = StrT::CopyLen(szTemp, pszFilePath, STRMAX(szTemp));
		FILECHAR_t* pszExt = GetFileNameExt(szTemp, iLen);
		if (pszExt != nullptr)
		{
			iLen = StrT::Diff(pszExt, szTemp);
		}

		StrT::CopyLen(szTemp + iLen, pszExtNew, STRMAX(szTemp) - iLen);
		return szTemp;
	}

	FILECHAR_t* GRAYCALL CFilePath::GetFileName(const FILECHAR_t* pszPath, StrLen_t iLen)	// static
	{
		//! Get file name and ext. (not path or drive) (File title)
		//! using iLen we might back up to the directory under any other directory.
		//! similar to COMMDLG.H GetFileTitleA(const char*, char*, WORD)

		if (pszPath == nullptr)
			return nullptr;
		if (iLen <= k_StrLen_UNK)
		{
			iLen = StrT::Len(pszPath);
		}
		StrLen_t iStart = iLen;
		while (iStart > 0)
		{
			iStart--;
			if (IsCharDirSep(pszPath[iStart]) || pszPath[iStart] == ':')
			{
				iStart++;
				break;
			}
		}
		pszPath += iStart;
		return(const_cast<FILECHAR_t*>(pszPath));
	}

	bool GRAYCALL CFilePath::HasTitleWildcards(const FILECHAR_t* pszPath) // static
	{
		//! Any wildcards ?* in this title ?
		//! Use StrT::MatchRegEx and CFileDir to evaluate these.

		if (pszPath == nullptr)
			return false;
		FILECHAR_t* pszTitle = GetFileName(pszPath);
		if (pszTitle == nullptr)
			return false;
		for (StrLen_t i = 0; pszTitle[i] != '\0'; i++)
		{
			if (IsCharWildcard(pszTitle[i]))
				return true;
		}
		return false;
	}

	CStringF GRAYCALL CFilePath::GetFileNameNE(const FILECHAR_t* pszPath, StrLen_t iLenPath, bool bMultiDot)	// static
	{
		//! Get the file name title with NO extension and NO path.
		FILECHAR_t szTmp[_MAX_PATH];
		StrLen_t iLenTmp = StrT::CopyLen(szTmp, GetFileName(pszPath, iLenPath), STRMAX(szTmp));
		StripFileExt(szTmp, iLenTmp, bMultiDot);
		return szTmp;
	}

	StrLen_t GRAYCALL CFilePath::MakeFileSymbolicName(ATOMCHAR_t* pszOut, const FILECHAR_t* pszPath, const ATOMCHAR_t* pszPrefix, ATOMCHAR_t chSub, bool bAllowLeadingNumber) // static
	{
		//! make a symbolic name from a file name. replace directory separators with _ chSub
		//! @note pszPrefix = nullptr is allowed.
		//! Limit pszOut to StrT::k_LEN_MAX_KEY
		//! @return
		//!  length of the string.

		ASSERT_N(pszOut != nullptr);
		if (pszPath == nullptr)
		{
			*pszOut = '\0';
			return 0;
		}

		StrLen_t iLenPrefix = StrT::CopyLen(pszOut, pszPrefix, StrT::k_LEN_MAX_KEY - 1);	// add the prefix.

		if (!StrT::CmpIN(pszPath, StrArg<FILECHAR_t>(pszOut), iLenPrefix))	// pszPath already prefixed. (don't dupe prefix!)
		{
			pszPath += iLenPrefix;
		}

		StrLen_t iLen = iLenPrefix;
		StrLen_t iLenLastDot = k_StrLen_UNK;	// last dot i found.

		StrLen_t iSrc = 0;
		while (iLen < StrT::k_LEN_MAX_KEY - 1 && iSrc < (StrT::k_LEN_MAX_KEY * 2))
		{
			FILECHAR_t ch = pszPath[iSrc++];
			if (ch == '\0')
				break;
			if (ch == ':')
			{
				// full path with drive is bad !!! get rid of drive info.
				// DEBUG_ERR(( "Drive in path?!" ));
				iLen = iLenPrefix;	// start over.
				iLenLastDot = k_StrLen_UNK;
				continue;
			}
			else if (ch == '.')	// dots in name
			{
				if (iLen == iLenPrefix)	// skip it at start. just part of the name (thats odd but acceptable)
					continue;
				if (iLenLastDot == iLen - 1) // relative dir ? ( if .. ) should collapse this back to the previous slash?
				{
					// Go back to last slash if there is one? MakeProperPath() ?
					// DEBUG_ERR(( "Relative path?!" ));
					continue;
				}
				iLenLastDot = iLen;
				ch = chSub;
			}
			else if (IsCharDirSep(ch))
			{
				if (iLen == iLenPrefix)	// skip it. root based ?
					continue;
				ch = chSub;
			}
			else if (StrChar::IsDigit(ch))	// ! alphanumeric
			{
				// don't allow numbers at start
				if (iLen == iLenPrefix && iLenPrefix == 0 && !bAllowLeadingNumber)	// skip it. can't lead with number. ignore prefix as it may not always be used?
					ch = chSub;
			}
			else if (!StrChar::IsAlpha(ch) && ch != '_')	// IsCSym
			{
				ch = chSub; // not a valid symbolic char!
			}
			if (ch == '\0')	// just skip it.
				continue;
			pszOut[iLen++] = (ATOMCHAR_t)ch;	// only a valid symbolic char
		}

		if (iLenLastDot > 0)	// strip extension.
		{
			iLen = iLenLastDot;	// just the last ext. other dots in the name are OK.
		}
		pszOut[iLen] = '\0';
		return iLen;
	}

	cStringA GRAYCALL CFilePath::GetFileSymbolicName(const FILECHAR_t* pszPath, const ATOMCHAR_t* pszPrefix, ATOMCHAR_t chSub, bool bAllowLeadingNumber) // static
	{
		//! convert the file name into a useful symbolic name. (SYMNAME)
		//! remove the directory and extension name.
		ATOMCHAR_t szTmp[StrT::k_LEN_MAX_KEY];
		StrLen_t iLen = MakeFileSymbolicName(szTmp, pszPath, pszPrefix, chSub, bAllowLeadingNumber);
		return cStringA(szTmp, iLen);
	}

	StrLen_t GRAYCALL CFilePath::MakeFullPath2(FILECHAR_t* pszFileOut, const FILECHAR_t* pszFileInp, StrLen_t iLenMax, FILECHAR_t chSep) // static
	{
		//! ASSERT(!IsFilePathRooted(pszFileInp))
		//! @note UNDER_CE has no concept of application current directory. All paths are rooted.
		//! @return Length
		StrLen_t iLen = CAppState::GetCurrentDir(pszFileOut, iLenMax);
		return CombineFilePathA(pszFileOut, iLenMax, iLen, pszFileInp, chSep);
	}

	StrLen_t GRAYCALL CFilePath::MakeFullPath(FILECHAR_t* pszFileOut, const FILECHAR_t* pszFileInp, StrLen_t iLenMax, FILECHAR_t chSep) // static
	{
		//! If this is a relative path (to app current dir) make this an absolute (rooted) path
		//! @return Length

		if (IsFilePathRooted(pszFileInp))
		{
			// its already rooted. leave it.
			return StrT::CopyLen(pszFileOut, pszFileInp, iLenMax);
		}

		// Was relative to the apps current dir. Get true full path.
		return MakeFullPath2(pszFileOut, pszFileInp, iLenMax, chSep);
	}

	CStringF GRAYCALL CFilePath::MakeFullPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep) // static
	{
		//! If this is a relative path (to app current dir) make this an absolute (rooted) path

		if (IsFilePathRooted(pszFileInp))
		{
			// its already rooted. leave it.
			return pszFileInp;
		}

		// Was relative to the apps current directory. Get true full path.
		FILECHAR_t szTmp[_MAX_PATH];
		StrLen_t nLen = MakeFullPath2(szTmp, pszFileInp, STRMAX(szTmp), chSep);
		UNREFERENCED_PARAMETER(nLen);
		return szTmp;
	}

	StrLen_t GRAYCALL CFilePath::AddFileDirSep(FILECHAR_t* pszOut, StrLen_t iLen, FILECHAR_t chSep)
	{
		//! Add the / or \ to the end to make this a directory.
		//! ASSUME pszOut >= _MAX_PATH
		//! Might be LINUX
		//! @return Length

		ASSERT_N(pszOut != nullptr);
		if (iLen < 0)
		{
			iLen = StrT::Len(pszOut);
		}
		if (iLen <= 0)
		{
			// DONT make root.
			return 0;
		}
		// can't add if too long.
		if (iLen < _MAX_PATH && !IsCharDirSep(pszOut[iLen - 1]))
		{
			pszOut[iLen++] = chSep;
			pszOut[iLen] = '\0';
		}
		return iLen;
	}

	StrLen_t GRAYCALL CFilePath::CombineFilePathA(FILECHAR_t* pszOut, StrLen_t iLenMax, StrLen_t iLen, const FILECHAR_t* pszName, FILECHAR_t chSep) // static
	{
		//! Append file/subdir pszName to existing pszOut path.
		//! @arg chSep = k_DirSep default.
		//! @return New total Length

		if (iLen < 0)
		{
			iLen = StrT::Len(pszOut);
		}
		if (pszName == nullptr)
		{
			return iLen;
		}
		if (iLen > 0)	// add a dir separator, but don't make root.
		{
			if (iLen < iLenMax - 1)
			{
				iLen = AddFileDirSep(pszOut, iLen, chSep);
			}
			while (IsCharDirSep(pszName[0]))
				pszName++;
		}
		return iLen + StrT::CopyLen(pszOut + iLen, SkipRelativePrefix(pszName), iLenMax - iLen);
	}

	StrLen_t GRAYCALL CFilePath::CombineFilePath(FILECHAR_t* pszOut, StrLen_t iLenMax, const FILECHAR_t* pszDir, const FILECHAR_t* pszName, FILECHAR_t chSep) // static
	{
		//! combine pszDir and pszName to make a single path. MakeProperPath.
		//! Similar to Shell PathAppend()
		//! .NET System.IO.Path.Combine
		//! @arg chSep = k_DirSep default.
		//! @return
		//!  length of full output string.
		//!  pszOut = pszDir + k_DirSep + pszName
		//! @note Resolve all relativism's in MakeProperPath

		ASSERT(pszOut != nullptr);
		ASSERT(pszOut != pszName);
		StrLen_t iLen = (pszDir == nullptr) ? k_StrLen_UNK : StrT::CopyLen(pszOut, pszDir, iLenMax);
		iLen = CombineFilePathA(pszOut, iLenMax, iLen, pszName, chSep);
		return MakeProperPath(pszOut, iLenMax, pszOut, chSep);
	}

	CStringF GRAYCALL CFilePath::CombineFilePathX(const FILECHAR_t* pszDir, const FILECHAR_t* pszName, FILECHAR_t chSep) // static
	{
		//! Merge path and file name. MakeProperPath.
		//! Similar to Shell PathAppend()
		//! .NET System.IO.Path.Combine
		//! @arg chSep = k_DirSep default.
		FILECHAR_t szFilePath[_MAX_PATH];
		CombineFilePath(szFilePath, _MAX_PATH, pszDir, pszName, chSep);
		return szFilePath;
	}

	CStringF _cdecl CFilePath::CombineFilePathF(FILECHAR_t chSep, const FILECHAR_t* pszBase, ...) // static
	{
		//! Combine a list of file name parts together. MUST be nullptr terminated list.
		//! @arg chSep = k_DirSep
		//! @arg pszBase = first entry in the list. nullptr terminated list.

		FILECHAR_t szFilePath[_MAX_PATH];
		StrLen_t iLen = StrT::CopyLen(szFilePath, pszBase, STRMAX(szFilePath));

		va_list vargs;
		va_start(vargs, pszBase);
		for (int iCount = 0; iCount < 64; iCount++)
		{
			const FILECHAR_t* pszPart = va_arg(vargs, const FILECHAR_t*);
			if (pszPart == nullptr)
				break;
			iLen = CombineFilePathA(szFilePath, STRMAX(szFilePath) - iLen, iLen, pszPart, chSep);
		}
		va_end(vargs);
		iLen = MakeProperPath(szFilePath, STRMAX(szFilePath), nullptr, chSep);
		return szFilePath;
	}

	bool GRAYCALL CFilePath::IsRelativeRoot(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir, StrLen_t iLenRootDir) // static
	{
		//! is the pszRootDir inside pszFullPath?
		//! e.g. pszFullPath="a\b\c", pszRootDir="a" = true
		if (pszFullPath == nullptr)
			return false;
		if (iLenRootDir < 0)
		{
			iLenRootDir = StrT::Len(pszRootDir);
		}
		if (!ComparePath(pszFullPath, pszRootDir, iLenRootDir))
			return true;
		return false;
	}

	bool GRAYCALL CFilePath::IsRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRelativePath) // static
	{
		//! a reverse compare of 2 paths. is pszRelativePath the same as pszFullPath assuming a root.
		//! e.g. pszFullPath="a\b\c", pszRelativePath="b\c" = true

		StrLen_t iLenFullPath = StrT::Len(pszFullPath);
		StrLen_t iLenRelativePath = StrT::Len(pszRelativePath);
		if (iLenRelativePath > iLenFullPath)
			return false;
		if (!ComparePath(pszFullPath + (iLenFullPath - iLenRelativePath), pszRelativePath, iLenRelativePath))
			return true;
		return false;
	}

	CStringF GRAYCALL CFilePath::MakeRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir) // static
	{
		//! given a pszFullPath, subtract pszRootDir out to make a relative path.
		//! If pszFullPath is not relative to pszRootDir just return the pszFullPath.
		//! Try using IsRelativeRoot() before call to this.
		//! e.g. pszFullPath = "c:\data\a\b\c.txt", pszRootDir = "c:\data", Return= "a\b\c.txt"

		if (StrT::IsNullOrEmpty(pszRootDir))
		{
			// nothing here.
			return pszFullPath;
		}

		StrLen_t iLen;

		// pszDir may be relative to the current path.
		FILECHAR_t szWorkDir[_MAX_PATH];
		if (IsFilePathRooted(pszRootDir))
		{
			// pszRootDir is absolute path
		}
		else
		{
			// pszRootDir isn't absolute path. make it one.
			iLen = StrT::Len(pszRootDir);
			if (!ComparePath(pszFullPath, pszRootDir, iLen))	// shortcut test.
			{
				// yes pszFullPath is in pszRootDir. take a shortcut.
				if (IsCharDirSep(pszFullPath[iLen]))
					iLen++;
				return(pszFullPath + iLen);
			}

			// So try the Current Working Directory full path next!
			MakeFullPath2(szWorkDir, pszRootDir, STRMAX(szWorkDir));
			pszRootDir = szWorkDir;
		}

		// IsRelativeRoot
		iLen = StrT::Len(pszRootDir);
		if (!ComparePath(pszFullPath, pszRootDir, iLen))
		{
			// Skip DirSep
			pszFullPath += iLen;
			if (IsCharDirSep(pszFullPath[0]))
				pszFullPath++;
		}
		else
		{
			// pszFullPath is not relative path for pszRootDir. just return full path. or its already a relative path ?
		}

		return pszFullPath;
	}

	COMPARE_t GRAYCALL CFilePath::ComparePath(const FILECHAR_t* pszName1, const FILECHAR_t* pszName2, StrLen_t iLenMax) // static
	{
		//! Compare 2 paths. equate \ and //
		//! @note file Path/Name may or may not be case sensitive! Linux = sensitive , Windows/Dos = not case sensitive.
		//! Try to factor out relatives .. ? using MakeFullPath() and MakeProperPath ?
		//! try to equate devices ? C: == /Device/Disk001 ?
		//! @arg
		//!  iLenMax=_MAX_PATH
		//! @return
		//!  0 = same. COMPARE_Equal
		//!

		if (pszName1 == nullptr || pszName2 == nullptr)
		{
			if (pszName1 == nullptr && pszName2 == nullptr)
			{
				return COMPARE_Equal;
			}
			if (pszName2 == nullptr)
				return COMPARE_Greater;
			return COMPARE_Less;
		}

		if (iLenMax < 0)
			iLenMax = _MAX_PATH;
		for (StrLen_t i = 0; i < iLenMax; i++)
		{
#ifdef __linux__
			FILECHAR_t ch1 = pszName1[i];
			FILECHAR_t ch2 = pszName2[i];
#else
			FILECHAR_t ch1 = (FILECHAR_t)StrChar::ToLowerA(pszName1[i]);
			FILECHAR_t ch2 = (FILECHAR_t)StrChar::ToLowerA(pszName2[i]);
#endif
			if (ch1 == ch2)
			{
				if (ch2 == '\0') // at end
					return COMPARE_Equal;	// same
				continue;
			}
			// different.
			if (IsCharDirSep(ch1))	// these are the same for the purposes of the compare.
				ch1 = k_DirSep;
			if (IsCharDirSep(ch2))
				ch2 = k_DirSep;
			if (ch1 != ch2)
				return(ch1 - ch2);
		}
		return COMPARE_Equal;	// they are the same.
	}

	FILECHAR_t* GRAYCALL CFilePath::SkipRelativePrefix(const FILECHAR_t* pszName) // static
	{
		//! if it has a relative prefix (.\) then skip it. it means nothing.
		while (pszName[0] == '.' && IsCharDirSep(pszName[1]))
		{
			pszName += 2;
		}
		return const_cast<FILECHAR_t*>(pszName);
	}

	FILECHAR_t* GRAYCALL CFilePath::GetFilePathUpDir2(const FILECHAR_t* pszName, StrLen_t iLen /*= k_StrLen_UNK*/, int iQtyDirs /*= 1*/) // static
	{
		//! go up this many folders if possible.
		//! iQtyDirs = 1 = the folder for "sdf:/dir1/dir2/dir3/dir4" = "/dir4"
		//!  2 = for "sdf:/dir1/dir2/dir3/dir4" = "/dir3/dir4"
		//!  -1 = for "sdf:/dir1/dir2/dir3/dir4" = "/dir2/dir3/dir4"
		//! ASSUME: trailing \ is a separate directory. weird.
		//! @return
		//!  pointer to char after the DirSep or 0th.
		//!  nullptr = cant go that far.

		if (pszName == nullptr)
		{
			return nullptr;
		}
		if (iQtyDirs == 0)
		{
			return const_cast<FILECHAR_t*>(pszName);
		}

		if (iQtyDirs < 0)
		{
			// go up from the bottom.
			if (iLen < 0)
			{
				iLen = _MAX_PATH;
			}
			// Device name doesn't count.
			StrLen_t i = GetFilePathDeviceLen(pszName);
			for (; i < iLen; i++)
			{
				FILECHAR_t ch = pszName[i];
				if (ch == '\0')
					break;
				if (IsCharDirSep(ch))
				{
					if (++iQtyDirs >= 0)
					{
						return const_cast<FILECHAR_t*>(pszName + i + 1);
					}
				}
			}
			return nullptr;
		}

		if (iLen < 0)
		{
			iLen = StrT::Len(pszName);
		}

		const FILECHAR_t* pszAct = pszName + iLen - 1;
		for (; pszAct >= pszName; pszAct--)
		{
			if (IsCharDirSep(pszAct[0]))
			{
				if (--iQtyDirs <= 0)
				{
					pszAct++;
					return const_cast<FILECHAR_t*>(pszAct);
				}
			}
		}

		if (iQtyDirs)
		{
			// cant go below root !
			if (IsFilePathRooted(pszName))
				return nullptr;
			if (iQtyDirs > 1)
				return nullptr;
		}
		return const_cast<FILECHAR_t*>(pszName);
	}

	CStringF GRAYCALL CFilePath::GetFilePathUpDir1(const FILECHAR_t* pszName, StrLen_t iLen /*= k_StrLen_UNK*/, int iQtyDirs /*= 1*/) // static
	{
		//! Go up a single dir.
		//! @arg iQtyDirs = -1 = for "sdf:/dir1/dir2/dir3/dir4" = "sdf:/dir1/dir2/dir3"
		const FILECHAR_t* pszAct = GetFilePathUpDir2(pszName, iLen, iQtyDirs);
		if (pszAct == nullptr)
			return "";
		return CStringF(pszName, StrT::Diff(pszAct, pszName) - 1);
	}

	bool GRAYCALL CFilePath::MakeFilePathUpDir(FILECHAR_t* pszName)	// static
	{
		//! Get the file path if the file were up one directory. in its parent dir.
		//! Like using ExtractDir()

		FILECHAR_t* pszTitle = GetFileName(pszName);
		if (pszTitle == nullptr)
			return false;
		FILECHAR_t* pszAct = GetFilePathUpDir2(pszName, StrT::Diff(pszTitle, pszName), 2);
		if (pszAct == nullptr)
			return false;
		CMem::CopyOverlap(pszAct, pszTitle, (StrT::Len(pszTitle) + 1) * sizeof(FILECHAR_t));	// restore file name/title + '\0'
		return true;
	}

	StrLen_t GRAYCALL CFilePath::MakeProperPath(FILECHAR_t* pszFileOut, StrLen_t iLenMax, const FILECHAR_t* pszFileInp, FILECHAR_t chSep) // static
	{
		//! Make sure all forward/back slashes are chSep.
		//! Remove/resolve relatives like ../ or ./ except if relativism would make us lower than root, then just leave it. like _WIN32 PathCanonicalize()
		//! _WIN32 calls use paths using \ backslashes. (default) __linux__ uses HTTP style forward / slashes.
		//! Windows typically can use either / or \ but they should be consistent.
		//! @arg
		//!  pszFileOut = (inplace pszFileOut == pszFileInp is ok)
		//!  iLenMax
		//!  chSep = / = to use / k_DirSep (default)
		//! @return
		//!  Length of pszFileOut

		ASSERT(IsCharDirSep(chSep));
		ASSERT_N(pszFileOut != nullptr);

		if (pszFileInp == nullptr)	// in place.
			pszFileInp = pszFileOut;
		bool bCopy = (pszFileOut != pszFileInp);	// in place fix is OK.

		int iFolders = 0;
		StrLen_t iOut = 0;

		while (iOut < iLenMax)
		{
			FILECHAR_t ch = *(pszFileInp++);
			FILECHAR_t chNew = ch;
			if (ch == '.')
			{
				StrLen_t iCountDots = 0;
				while (pszFileInp[iCountDots] == '.')
					iCountDots++;
				if (IsCharDirSep(pszFileInp[iCountDots]))	// ../stuff
				{
					// Back up a directory if we can.
					iCountDots++;
					FILECHAR_t* pszUpDir = GetFilePathUpDir2(pszFileOut, iOut, iCountDots);
					if (pszUpDir == nullptr)	// iCountDots > iFolders
					{
						// Just preserve it since it cant be resolved?
						iCountDots--;
						if (bCopy)
						{
							iOut += StrT::CopyLen(pszFileOut + iOut, pszFileInp - 1, MIN(iCountDots + 2, iLenMax - iOut));
						}
						else
						{
							iOut += iCountDots + 1;
						}
					}
					else
					{
						iOut = StrT::Diff(pszUpDir, pszFileOut);
						bCopy = true;
					}
					pszFileInp += iCountDots;	// skip over it.
					continue;
				}
			}

			if (IsCharDirSep(ch))
			{
				iFolders++;
				chNew = chSep;
			}
			if (bCopy || chNew != ch)
			{
				pszFileOut[iOut] = chNew;
			}
			if (ch == '\0')	// done.
				break;
			iOut++;
		}
		return iOut;
	}

	CStringF GRAYCALL CFilePath::MakeProperPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep) // static
	{
		//! Make sure all forward/back slashes are chSep.
		FILECHAR_t szFilePath[_MAX_PATH];
		MakeProperPath(szFilePath, STRMAX(szFilePath), pszFileInp, chSep);
		return szFilePath;
	}

	StrLen_t GRAYCALL CFilePath::ExtractDir(FILECHAR_t* pszFilePath, StrLen_t iLen, bool bTrailingSep) // static
	{
		//! Remove the file name from this and just leave the path.
		//! bTrailingSep = leave the trailing /
		//! opposite of GetFileName()
		//! @return
		//!  length of the new string.

		FILECHAR_t* pszTitle = GetFileName(pszFilePath, iLen);

		for (; pszTitle > pszFilePath; pszTitle--)
		{
			if (*pszTitle == ':')	// has a drive letter.
			{
				pszTitle++;
				break;
			}
			if (IsCharDirSep(*pszTitle))	// Might be LINUX
			{
				if (bTrailingSep)
					pszTitle++;
				break;
			}
		}
		*pszTitle = '\0';
		return StrT::Diff(pszTitle, pszFilePath);
	}

	StrLen_t GRAYCALL CFilePath::ExtractDirCopy(FILECHAR_t* pszDirPath, StrLen_t iLenMax, const FILECHAR_t* pszFilePathSrc, bool bTrailingSep) // static
	{
		//! Remove the file name from this and just leave the path. (was ExtractDir)
		//! @arg bTrailingSep = leave the trailing /
		//! @return
		//!  length of the new string.
		StrLen_t iLen = StrT::CopyLen(pszDirPath, pszFilePathSrc, iLenMax);
		return ExtractDir(pszDirPath, iLen, bTrailingSep);
	}

	CStringF GRAYCALL CFilePath::GetFileDir(const FILECHAR_t* pszPath, bool bTrailingSep) // static
	{
		//! Extract the directory from a file path. include the trailing / k_DirSep if bTrailingSep is set.
		//! @arg bTrailingSep = leave the trailing /
		FILECHAR_t szPath[_MAX_PATH];
		StrLen_t iLen = ExtractDirCopy(szPath, STRMAX(szPath), pszPath, bTrailingSep);
		UNREFERENCED_PARAMETER(iLen);
		return szPath;
	}

	bool GRAYCALL CFilePath::IsFileNameExt(const FILECHAR_t* pszFileName, const FILECHAR_t* pszExt) // static
	{
		//! Is this the extension for the file name ? with or without dot.
		if (pszExt == nullptr)
			return false;
		StrLen_t iLenExt = StrT::Len(pszExt);
		StrLen_t iLenName = StrT::Len(pszFileName);
		if (iLenExt >= iLenName)	// can't be. it wouldn't fit.
		{
			return false;
		}
		return !StrT::CmpIN(pszFileName + (iLenName - iLenExt), pszExt, iLenExt);
	}

	CStringF GRAYCALL CFilePath::GetNameExtStar(const FILECHAR_t* pszFilePath) // static
	{
		//! Convert a name possibly with a full path to a name and extension wildcard.
		//!  "dir/Name.ext"
		//! @return
		//!  "Name.*"
		static const FILECHAR_t* k_pszExt = _FN(".*");
		return ReplaceFileExt(GetFileName(pszFilePath), k_pszExt);
	}

#ifdef _WIN32
	const wchar_t* GRAYCALL CFilePath::MakeFileNameLongW(const FILECHAR_t* pszFilePath) // static 
	{
		//! Add _WIN32 k_NamePrefix if the filename is too long for the system call.
		if (StrT::StartsWithI<FILECHAR_t>(pszFilePath, k_NamePrefix))	// already prefixed.
			return StrArg<wchar_t>(pszFilePath);
		CStringF sPathNew = k_NamePrefix;
		sPathNew += pszFilePath;
		return  StrArg<wchar_t>(sPathNew);
	}
	const wchar_t* GRAYCALL CFilePath::GetFileNameLongW(CStringF sFilePath) // static 
	{
		//! Add _WIN32 k_NamePrefix if the filename is too long for the system call.
		if (sFilePath.GetLength() <= _MAX_PATH)	// short names don't need this.
			return StrArg<wchar_t>(sFilePath.get_CPtr());
		return MakeFileNameLongW(sFilePath);
	}
	const wchar_t* GRAYCALL CFilePath::GetFileNameLongW(const FILECHAR_t* pszFilePath) // static 
	{
		//! Add _WIN32 k_NamePrefix if the filename is too long for the system call.
		if (StrT::Len(pszFilePath) <= _MAX_PATH)	// short names don't need this.
			return StrArg<wchar_t>(pszFilePath);
		return MakeFileNameLongW(pszFilePath);
	}
#endif
}

//*******************************************************************

#ifdef USE_UNITTESTS
#include "CUnitTest.h"

UNITTEST_CLASS(CFilePath)
{
	UNITTEST_METHOD(CFilePath)
	{
		static const FILECHAR_t* k_UnitTest_FilePaths[] =
		{
			// test some odd file names.
			_FN("sdf-fdfs/./sdf.ext"),
			_FN("./sdf.ext"),
			_FN("/sdf-fdfs/../sdf.ext"),
			_FN("/../sdf.ext"),
			_FN("sdf-fdfs/../sdf.ext"),
			_FN("../abcd/c/d/e/../../ef.ext"),
			_FN("c:/..\\abcd\\ef.ext"),
			_FN("../abcd/ef.ext"),
			_FN("..\\abcd\\ef.ext"),
			_FN("goodname\\goodstuff.ext"),	// intentional bad char \g
			_FN("123123123.ext"),
			_FN("asd/123123123.ext"),
			_FN("sdf-fdfs___---_asd/123123123.ext"),
			_FN("abc.efg.hij.ext"),
			_FN("abc.efg.hij..ext"),
			_FN("c:\\Windows\\System32\\smss.exe"),
			_FN("\\Device\\HarddiskVolume2\\Windows\\System32\\smss.exe"),
			_FN("\\\\dennislap\\samplesx\\stuff.txt"),
			// "com1:
			// "\\.\PhysicalDisk1"
			// "\\?\UNC\" = long UNC name. ignores .. relatives.
			// "%SystemDir%", "%WinDir%", "%ProfileDir%", "%SystemRoot%" like REG_EXPAND_SZ ExpandEnvironmentStrings SHGetSpecialFolderPath() ?
			nullptr,
		};

		for (UINT i = 0; i < _countof(k_UnitTest_FilePaths) - 1; i++)
		{
			const FILECHAR_t* pszTest = k_UnitTest_FilePaths[i];

			// GetFileSymbolicName
			cStringA sSymbolicName(CFilePath::GetFileSymbolicName(pszTest, nullptr));
			sm_pLog->addDebugInfoF("File='%s', Symbol='%s'", LOGSTR(pszTest), LOGSTR(sSymbolicName));

			sSymbolicName = CFilePath::GetFileSymbolicName(pszTest, "PREFIX");
			UNITTEST_TRUE(sSymbolicName.GetLength());

			// MakeProperPath
			FILECHAR_t szOut[_MAX_PATH];
			StrLen_t nLen2 = CFilePath::MakeProperPath(szOut, STRMAX(szOut), pszTest);
			StrLen_t nLen1 = StrT::CopyLen(szOut, pszTest, STRMAX(szOut));
			StrLen_t nLen3 = CFilePath::MakeProperPath(szOut);

			UNITTEST_TRUE(nLen1 >= nLen2);
			UNITTEST_TRUE(nLen2 == nLen3);
		}

		FILECHAR_t* pszUpDir2 = CFilePath::GetFilePathUpDir2(_FN("/a/b/c/d/ef.ext"), k_StrLen_UNK, 2);
		UNITTEST_TRUE(!StrT::CmpI(pszUpDir2, _FN("d/ef.ext")));
		pszUpDir2 = CFilePath::GetFilePathUpDir2(_FN("/a/b/c/d/ef.ext"), k_StrLen_UNK, -2);
		UNITTEST_TRUE(!StrT::CmpI(pszUpDir2, _FN("b/c/d/ef.ext")));

		CStringF sUpDir1 = CFilePath::GetFilePathUpDir1(_FN("sdf:/dir1/dir2/dir3/dir4"), k_StrLen_UNK, 1);
		UNITTEST_TRUE(!sUpDir1.Compare(_FN("sdf:/dir1/dir2/dir3")));

		// combine and make proper.
		CStringF strFilePath = CFilePath::CombineFilePathF('/', _FN("a"), _FN("b\\c"), _FN("d/ef.ext"), nullptr);
		UNITTEST_TRUE(!strFilePath.Compare(_FN("a/b/c/d/ef.ext")));
	}
};
UNITTEST_REGISTER(CFilePath, UNITTEST_LEVEL_Core);
#endif
