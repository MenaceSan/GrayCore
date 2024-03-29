//! @file cFilePath.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrChar.h"
#include "StrConst.h"
#include "cAppState.h"
#include "cFileDir.h"
#include "cFilePath.h"
#include "cLogMgr.h"

#ifdef __linux__
// #include <ctype.h>
#elif defined(_WIN32) && !defined(UNDER_CE)
#include <direct.h>
#endif

namespace Gray {
#ifdef _WIN32
const FILECHAR_t cFilePath::k_NamePrefix[5] = _FN("\\\\?\\");  /// pre-pend "\\?\" to the path to extend this limit to 32,767 on NTFS.
#endif

FILECHR_MASK_t GRAYCALL cFilePath::GetFileCharType(wchar_t ch, FILESYS_t eSys) NOEXCEPT {  // static
    //! Is the char valid for a filename on FILECHR_MASK_t?
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

    static const char* k_pszBadChars = "<>\"|";              // chars NEVER allowed.
    static const char* k_pszAllowedDOS = "^$~!#%&-{}()@'`";  // old DOS names allowed.

    if (ch < 255) {
        if (ch == '\0')  // never allowed chars.
            return FILECHR_Invalid;
        if (ch < ' ') return FILECHR_Invalid;  // UTF8 UNICODE?
        if (StrChar::IsAlNum(ch))              // ASCII letter or number is always ok.
            return FILECHR_Name;
        if (IsCharWildcard(ch)) return FILECHR_Wildcard;
#if USE_UNICODE_FN
        if (ch <= 255)  // NOT UNICODE char
#endif
        {
            if (StrT::HasChar(k_pszBadChars, (char)ch))  // never allowed as name
                return FILECHR_Invalid;
            if (StrT::HasChar(k_pszAllowedDOS, (char)ch))  // old DOS names allowed chars.
                return FILECHR_Name2;                      // May need escape container.
        }
    }

    switch (ch) {
        case '_':  // always allowed non escaped.
            return FILECHR_Name;
        case '/':  // IsCharDirSep(ch)
            if (eSys == FILESYS_t::_FAT) return FILECHR_Invalid;
            return FILECHR_Dirs;
        case '\\':
            if (eSys == FILESYS_t::_NFS) return FILECHR_Name;
            return FILECHR_Dirs;
        case ':':
            return FILECHR_Device;
        case '.':
            // NFS first char uses this to hide files.
            return FILECHR_Ext;
        case '+':  // reserved for FAT32
        case '[':  // reserved for FAT32
        case ']':  // reserved for FAT32
            if (eSys == FILESYS_t::_NFS || eSys == FILESYS_t::_NTFS) return FILECHR_Name2;
            return FILECHR_Invalid;
        case ' ':
        case ',':
            if (eSys == FILESYS_t::_FAT) return FILECHR_Invalid;
            return FILECHR_Name2;
        case 0xA5:  // Yen symbol. Its a FILECHR_Dirs symbol in Japan ?? http://msdn.microsoft.com/en-us/library/windows/desktop/dd317748%28v=vs.85%29.aspx
            return FILECHR_Invalid;
    }

    if (eSys == FILESYS_t::_FAT) return FILECHR_Invalid;  // everything else is bad for old DOS FAT.

    return FILECHR_Name3;  // some other sort of char like foreign char. but allowed.
}

bool GRAYCALL cFilePath::IsFileNameValid(const FILECHAR_t* pszName, FILECHR_MASK_t uCharMask, FILESYS_t eSys) {  // static
    //! Is this a valid file name? Maybe UTF8.
    //! Do not end a file or directory name with a space or a period.
    //! @arg eSys = FILESYS_t::_FAT = enforce the DOS 8.3 rules.
    //! @note CANNOT have names the same as system devices. e.g. CLOCK$ CON PRN AUX NUL COM# LPT#. Check for those here ?

    if (pszName == nullptr) return false;

    StrLen_t i = 0;
    for (; pszName[i] != '\0'; i++) {
        FILECHAR_t ch = pszName[i];
        if (!(uCharMask & GetFileCharType(ch, eSys))) return false;
    }
    if (i > 0) {
        // Do not end a file or directory name with a space or a period
        FILECHAR_t ch = pszName[i - 1];
        if (ch == '.' || ch == ' ') return false;
    }
    return true;
}

bool GRAYCALL cFilePath::IsFilePathTitle(const FILECHAR_t* pszName) {  // static
    //! Does this NOT have path/dir indicators?
    if (pszName == nullptr) return false;
    for (StrLen_t i = 0;; i++, pszName++) {
        if (i > _MAX_PATH - 1) break;  // weird.
        FILECHAR_t ch = pszName[0];
        if (ch == '\0') return true;
        if (IsCharDirSep(ch)) break;  // this sort of access is not allowed.            
        if (ch == ':') break;  // this sort of access is not allowed.            
    }
    return false;
    ;
}

bool GRAYCALL cFilePath::HasFilePathRelatives(const FILECHAR_t* pszName, bool bOrDevices) {  // static
    //! Does the file have any relative components. like ..
    //! Get rid of them with MakeProperPath()

    if (pszName == nullptr) return false;
    for (StrLen_t i = 0;; i++, pszName++) {
        if (i > _MAX_PATH - 1) break;  // weird.
        FILECHAR_t ch = pszName[0];
        if (ch == '\0') return false;
        if (ch == '.' && pszName[1] == '.')  // has relative path ../ of some sort. ./ does not count ?
            break;
        if (bOrDevices) {
            if (IsCharDirSep(ch) && IsCharDirSep(pszName[1]))  // this sort of access is not allowed.
                break;
            if (ch == ':')  // this sort of access is not allowed.
                break;
        }
    }
    return true;
}

StrLen_t GRAYCALL cFilePath::GetFilePathDeviceLen(const FILECHAR_t* pszName) {  // static
    //! Skip the device info at the start of the path.
    //! have a device name in it ? e.g. "COM1:" or "C:"
    //! "file://", "http://"

    if (pszName == nullptr) return 0;

#ifdef _WIN32
    // Special _WIN32 format?
    if (!StrT::CmpI(pszName, _FN("\\Device\\"))) {
        // TODO: Handle \\Device\\style ;
    }
#endif

    for (StrLen_t i = 0;; i++) {
        FILECHAR_t ch = pszName[i];
        if (ch == ':')  // end of device name.
        {
            if (i == 0) break;
            return i + 1;  // e.g. "c:" = 2
        }
        if (!StrChar::IsAlpha(ch))  // not valid device name?
            break;
    }
    return 0;
}

bool GRAYCALL cFilePath::IsFileDeviceRemote(const FILECHAR_t* pszPath) {  // static
    //! Is the file based on some remote device/service? e.g. HTTP, HTTPS, FTP, RTP, RTMP etc.
    //! GetFilePathDeviceLen
    //! like NETSERVICE_TYPE

    if (!StrT::CmpIN(pszPath, _FN("file:"), 5))  // local file system.
        return false;
    StrLen_t nLenDev = GetFilePathDeviceLen(pszPath);
    if (nLenDev <= 2)  // just a local drive letter. Rooted.
        return false;
    return true;  // must be a remote protocol.
}

bool GRAYCALL cFilePath::IsFilePathRooted(const FILECHAR_t* pszName) {  // static
    //! Is the file path absolute ? (not relative path to current directory for process)
    //! Based on drive, device or root ? not have .. in it.
    //! If relative path then use MakeFullPathX() to get full rooted path.
    //! e.g. "COM1:" is true!
    //! "C:\" is true.

    if (StrT::IsNullOrEmpty(pszName)) return false;
    // starts with '\\'
    StrLen_t iLen = GetFilePathDeviceLen(pszName);
    if (iLen > 0 && pszName[iLen] == '\0') return true;
    if (IsCharDirSep(pszName[iLen])) return true;
    return false;
}

bool GRAYCALL cFilePath::IsFilePathRoot(const FILECHAR_t* pszName) {  // static
    //! is this the root of a device?
    //! @note Includes the DOT !
    //! e.g. "C:\" is true
    if (StrT::IsNullOrEmpty(pszName)) return false;
    StrLen_t iLen = GetFilePathDeviceLen(pszName);
    if (pszName[iLen] == '\0') return false;
    // end with '\\'
    if (IsCharDirSep(pszName[iLen]) && pszName[iLen + 1] == '\0') return true;
    return false;
}

FILECHAR_t* GRAYCALL cFilePath::GetFileNameExt(const FILECHAR_t* pszName, StrLen_t iLen, bool bMultiDot) {  // static
    //! Get a pointer to the extension of the file.
    //! @note Includes the DOT !
    //! @arg
    //!	iLen = the known string length of the file name. -1 = k_StrLen_UNK = find '\0'
    //!	bMultiDot = ".xt.sdf.fff" is considered a single extension.
    //! @return
    //!  The pointer to the extension in this file name. Includes the DOT.
    //!  nullptr = has no extension.

    if (pszName == nullptr) return nullptr;
    if (iLen <= k_StrLen_UNK) {
        iLen = StrT::Len(pszName);
    }
    FILECHAR_t* pszExt = nullptr;  // has no valid ext.
    while (iLen > 0) {
        iLen--;
        FILECHAR_t ch = pszName[iLen];
        if (ch == '.') {
            pszExt = (FILECHAR_t*)(pszName + iLen);
            if (bMultiDot) continue;
            break;
        }
        // FILECHR_Device|FILECHR_Dirs
        if (IsCharDirSep(ch) || ch == ':') break;
        if (!(GetFileCharType(ch) & FILECHR_Name))  // valid ext name character?
            break;
    }
    return pszExt;
}

StrLen_t GRAYCALL cFilePath::StripFileExt(cSpanX<FILECHAR_t>& fileName, bool bMultiDot) {  // static
    //! strip the ext off the file name (or path).
    //! @arg
    //!	iLen = the known string length of the file name.
    //! @return
    //!	new length of the string.
    if (fileName.isNull()) return 0;
    FILECHAR_t* pszExt = GetFileNameExt(fileName, fileName.get_MaxLen(), bMultiDot);
    if (StrT::IsNullOrEmpty(pszExt)) {
        return fileName.get_MaxLen();
    }
    ASSERT(fileName.IsInternalPtr(pszExt));
    *pszExt = '\0';
    return StrT::Diff(pszExt, fileName.get_DataConst());
}

cFilePath GRAYCALL cFilePath::ReplaceFileExt(const FILECHAR_t* pszFilePath, const FILECHAR_t* pszExtNew) {
    //! Replace the existing Ext with this new one. If it didn't have one then just add this.
    //! @arg pszExtNew = ".EXT" (with dot)

    if (pszFilePath == nullptr) return "";

    FILECHAR_t szTemp[_MAX_PATH];
    StrLen_t iLen = StrT::Copy(TOSPAN(szTemp), pszFilePath);
    FILECHAR_t* pszExt = GetFileNameExt(szTemp, iLen);
    if (pszExt != nullptr) {
        iLen = StrT::Diff(pszExt, szTemp);
    }
    StrT::Copy(ToSpan(szTemp + iLen, STRMAX(szTemp) - iLen), pszExtNew);
    return szTemp;
}

FILECHAR_t* GRAYCALL cFilePath::GetFileName(const FILECHAR_t* pszPath, StrLen_t iLen) {  // static
    if (pszPath == nullptr) return nullptr;
    if (iLen <= k_StrLen_UNK) iLen = StrT::Len(pszPath);
    StrLen_t iStart = iLen;
    while (iStart > 0) {
        iStart--;
        if (IsCharDirSep(pszPath[iStart]) || pszPath[iStart] == ':') {
            iStart++;
            break;
        }
    }
    pszPath += iStart;
    return const_cast<FILECHAR_t*>(pszPath);
}

bool GRAYCALL cFilePath::HasTitleWildcards(const FILECHAR_t* pszPath) {  // static
    //! Any wildcards ?* in this title ?
    //! Use StrT::MatchRegEx and cFileDir to evaluate these.

    if (pszPath == nullptr) return false;
    FILECHAR_t* pszTitle = GetFileName(pszPath);
    if (pszTitle == nullptr) return false;
    for (StrLen_t i = 0; pszTitle[i] != '\0'; i++) {
        if (IsCharWildcard(pszTitle[i])) return true;
    }
    return false;
}

cFilePath GRAYCALL cFilePath::GetFileNameNE(const FILECHAR_t* pszPath, StrLen_t iLenPath, bool bMultiDot) {  // static
    // Title, GetFileTitle()
    FILECHAR_t szTmp[_MAX_PATH];
    const StrLen_t iLenTmp = StrT::Copy(TOSPAN(szTmp), GetFileName(pszPath, iLenPath));
    StripFileExt(ToSpan(szTmp, iLenTmp), bMultiDot);
    return szTmp;
}

StrLen_t GRAYCALL cFilePath::MakeSymName(ATOMCHAR_t* pszOut, const FILECHAR_t* pszPath, ATOMCHAR_t chSub, BYTE flags) {  // static
    // like cAtomRef::MakeSymName
    ASSERT_NN(pszOut);
    if (pszPath == nullptr) {
        *pszOut = '\0';
        return 0;
    }

    StrLen_t iLen = 0;
    StrLen_t iLenLastDot = k_StrLen_UNK;  // last dot i found.

    StrLen_t iSrc = 0;
    while (iLen < k_LEN_MAX_CSYM - 1 && iSrc < (k_LEN_MAX_CSYM * 2)) {
        FILECHAR_t ch = pszPath[iSrc++];
        if (ch == '\0') break;
        if (ch == ':') {
            // full path with drive is bad !!! get rid of drive info.
            // DEBUG_ERR(( "Drive in path?!" ));
            iLen = 0;  // start over.
            iLenLastDot = k_StrLen_UNK;
            continue;
        } else if (ch == '.') {             // dots in name
            if (iLen == 0) continue;        // skip it at start. just part of the name (thats odd but acceptable)
            if (iLenLastDot == iLen - 1) {  // relative dir ? ( if .. ) should collapse this back to the previous slash?
                // Go back to last slash if there is one? MakeProperPath() ?
                // DEBUG_ERR(( "Relative path?!" ));
                continue;
            }
            iLenLastDot = iLen;
            ch = chSub;
        } else if (IsCharDirSep(ch)) {
            if (iLen == 0) continue;  // skip it. root based ?
            ch = chSub;
        } else if (StrChar::IsDigitA(ch)) {  // ! alphanumeric
            // don't allow numbers at start
            if (iLen == 0 && !(flags & FILECHR_Name2))  // skip it. can't lead with number. ignore prefix as it may not always be used?
                ch = chSub;
        } else if (!StrChar::IsAlpha(ch) && ch != chSub) {  // IsCSym
            ch = chSub;                                     // not a valid symbolic char!
        }
        if (ch == '\0') continue;                // just skip it.
        pszOut[iLen++] = CastN(ATOMCHAR_t, ch);  // only a valid symbolic char
    }

    if (iLenLastDot > 0 && !(flags & FILECHR_Ext)) {  // strip extension.
        iLen = iLenLastDot;                           // just the last ext. other dots in the name are OK.
    }
    pszOut[iLen] = '\0';
    return iLen;
}

cStringA GRAYCALL cFilePath::MakeSymNameStr(const FILECHAR_t* pszPath, ATOMCHAR_t chSub, BYTE flags) {  // static
    ATOMCHAR_t szTmp[k_LEN_MAX_CSYM];
    const StrLen_t iLen = MakeSymName(szTmp, pszPath, chSub, flags);
    return ToSpan(szTmp, iLen);
}

COMPARE_t GRAYCALL cFilePath::ComparePath(const FILECHAR_t* pszName1, const FILECHAR_t* pszName2, StrLen_t iLenMax) noexcept {  // static
    //! Compare 2 paths. equate \ and //
    //! @note file Path/Name may or may not be case sensitive! Linux = sensitive , Windows/Dos = not case sensitive.
    //! Try to factor out relatives .. ? using MakeFullPath() and MakeProperPath ?
    //! try to equate devices ? C: == /Device/Disk001 ?
    //! @arg
    //!  iLenMax=_MAX_PATH
    //! @return
    //!  0 = same. COMPARE_Equal

    if (pszName1 == nullptr || pszName2 == nullptr) {
        if (pszName1 == nullptr && pszName2 == nullptr) {
            return COMPARE_Equal;
        }
        if (pszName2 == nullptr) return COMPARE_Greater;
        return COMPARE_Less;
    }

    if (iLenMax < 0) iLenMax = _MAX_PATH;
    for (StrLen_t i = 0; i < iLenMax; i++) {
#ifdef __linux__
        FILECHAR_t ch1 = pszName1[i];
        FILECHAR_t ch2 = pszName2[i];
#else
        FILECHAR_t ch1 = StrChar::ToLower(pszName1[i]);
        FILECHAR_t ch2 = StrChar::ToLower(pszName2[i]);
#endif
        if (ch1 == ch2) {
            if (ch2 == '\0')           // at end
                return COMPARE_Equal;  // same
            continue;
        }
        // different.
        if (IsCharDirSep(ch1))  // these are the same for the purposes of the compare.
            ch1 = k_DirSep;
        if (IsCharDirSep(ch2)) ch2 = k_DirSep;
        if (ch1 != ch2) {
            if (ch1 == k_DirSep && ch2 == '\0' && pszName1[i + 1] == '\0')  // special case for trailing k_DirSep.
                break;
            if (ch2 == k_DirSep && ch1 == '\0' && pszName2[i + 1] == '\0') break;
            return ch1 - ch2;
        }
    }
    return COMPARE_Equal;  // they are the same.
}

StrLen_t GRAYCALL cFilePath::MakeFullPath2(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFileInp, FILECHAR_t chSep) {  // static
    //! ASSERT(!IsFilePathRooted(pszFileInp))
    //! @note UNDER_CE has no concept of application current directory. All paths are rooted.
    //! @return Length
    const StrLen_t iLen = cAppState::GetCurrentDir(ret);
    return CombineFilePathA(ret, iLen, pszFileInp, chSep);
}

StrLen_t GRAYCALL cFilePath::MakeFullPath(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFileInp, FILECHAR_t chSep) {  // static
    //! If this is a relative path (to app current dir) make this an absolute (rooted) path
    //! @return Length

    if (IsFilePathRooted(pszFileInp)) {
        // its already rooted. leave it.
        return StrT::Copy(ret, pszFileInp);
    }

    // Was relative to the apps current dir. Get true full path.
    return MakeFullPath2(ret, pszFileInp, chSep);
}

cFilePath GRAYCALL cFilePath::MakeFullPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep) {  // static
    //! If this is a relative path (to app current dir) make this an absolute (rooted) path

    if (IsFilePathRooted(pszFileInp)) {
        // its already rooted. leave it.
        return pszFileInp;
    }

    // Was relative to the apps current directory. Get true full path.
    FILECHAR_t szTmp[_MAX_PATH];
    StrLen_t nLen = MakeFullPath2(TOSPAN(szTmp), pszFileInp, chSep);
    UNREFERENCED_PARAMETER(nLen);
    return szTmp;
}

StrLen_t GRAYCALL cFilePath::AddFileDirSep(FILECHAR_t* pszOut, StrLen_t iLen, FILECHAR_t chSep) {
    //! Add the / or \ to the end to make this a directory.
    //! ASSUME pszOut >= _MAX_PATH
    //! Might be LINUX
    //! @return Length

    ASSERT_NN(pszOut);
    if (iLen < 0) iLen = StrT::Len(pszOut);
    if (iLen <= 0) return 0;  // DONT make root.

    // can't add if too long.
    if (iLen < _MAX_PATH && !IsCharDirSep(pszOut[iLen - 1])) {
        pszOut[iLen++] = chSep;
        pszOut[iLen] = '\0';
    }
    return iLen;
}

/// <summary>
/// Remove the / or \ from the end of this directory.
/// reverse of AddFileDirSep()
/// </summary>
/// <param name="sDir"></param>
/// <returns></returns>
cFilePath GRAYCALL cFilePath::RemoveFileDirSep(const cStringF& sDir) {
    const StrLen_t len = sDir.GetLength();
    if (len <= 1) return sDir;
    const StrLen_t len2 = len - 1;
    if (!IsCharDirSep(sDir[len2])) return sDir;  // no change.
    return sDir.Left(len2);
}

StrLen_t GRAYCALL cFilePath::CombineFilePathA(cSpanX<FILECHAR_t>& ret, StrLen_t iLen, const FILECHAR_t* pszName, FILECHAR_t chSep) {  // static
    //! Append file/subdir pszName to existing pszFilePath path.
    //! @arg chSep = k_DirSep default.
    //! @return New total Length

    if (iLen <= k_StrLen_UNK) iLen = StrT::Len(ret.get_DataConst());
    if (pszName == nullptr) return iLen;

    if (iLen > 0) {  // add a dir separator, but don't make root.
        if (iLen < ret.get_MaxLen() - 1) {
            iLen = AddFileDirSep(ret.get_DataWork(), iLen, chSep);
        }
        while (IsCharDirSep(pszName[0])) pszName++;
    }
    return iLen + StrT::CopyLen(ret.get_DataWork() + iLen, SkipRelativePrefix(pszName), ret.get_MaxLen() - iLen);
}

StrLen_t GRAYCALL cFilePath::CombineFilePath(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszDir, const FILECHAR_t* pszName, FILECHAR_t chSep) {  // static
    //! combine pszDir and pszName to make a single path. MakeProperPath.
    //! Similar to Shell PathAppend()
    //! .NET System.IO.Path.Combine
    //! @arg chSep = k_DirSep default.
    //! @return
    //!  length of full output string.
    //!  pszOut = pszDir + k_DirSep + pszName
    //! @note Resolve all relativism's in MakeProperPath

    ASSERT(ret != pszName);

    // TODO Test if pszName already starts with pszDir ? and is rooted ?

    StrLen_t iLen = (pszDir == nullptr) ? k_StrLen_UNK : StrT::Copy(ret, pszDir);
    iLen = CombineFilePathA(ret, iLen, pszName, chSep);
    return MakeProperPath(ret, ret, chSep);
}

cFilePath GRAYCALL cFilePath::CombineFilePathX(const FILECHAR_t* pszDir, const FILECHAR_t* pszName, FILECHAR_t chSep) {  // static
    //! Merge path and file name. MakeProperPath.
    //! Similar to Shell PathAppend()
    //! .NET System.IO.Path.Combine
    //! @arg chSep = k_DirSep default.
    FILECHAR_t szFilePath[_MAX_PATH];
    CombineFilePath(TOSPAN(szFilePath), pszDir, pszName, chSep);
    return szFilePath;
}

cFilePath _cdecl cFilePath::CombineFilePathF(FILECHAR_t chSep, const FILECHAR_t* pszBase, ...) {  // static
    //! Combine a list of file name parts together. MUST be nullptr terminated list.
    //! @arg chSep = k_DirSep
    //! @arg pszBase = first entry in the list. nullptr terminated list.

    FILECHAR_t szFilePath[_MAX_PATH];
    StrLen_t iLen = StrT::Copy(TOSPAN(szFilePath), pszBase);

    va_list vargs;
    va_start(vargs, pszBase);
    for (int iCount = 0; iCount < k_ARG_ARRAY_MAX; iCount++) {  // arbitrary max.
        const FILECHAR_t* pszPart = va_arg(vargs, const FILECHAR_t*);
        if (pszPart == nullptr) break;
        iLen = CombineFilePathA(TOSPAN(szFilePath), iLen, pszPart, chSep);
    }
    va_end(vargs);

    iLen = MakeProperPath(TOSPAN(szFilePath), nullptr, chSep);
    return szFilePath;
}

bool GRAYCALL cFilePath::IsRelativeRoot(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir, StrLen_t iLenRootDir) {  // static
    //! is the pszRootDir inside pszFullPath?
    //! e.g. pszFullPath="a\b\c", pszRootDir="a" = true
    if (pszFullPath == nullptr) return false;
    if (iLenRootDir < 0) iLenRootDir = StrT::Len(pszRootDir);
    if (!ComparePath(pszFullPath, pszRootDir, iLenRootDir)) return true;
    return false;
}

bool GRAYCALL cFilePath::IsRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRelativePath) {  // static
    //! a reverse compare of 2 paths. is pszRelativePath the same as pszFullPath assuming a root.
    //! e.g. pszFullPath="a\b\c", pszRelativePath="b\c" = true

    ASSERT_NN(pszFullPath);
    StrLen_t iLenFullPath = StrT::Len(pszFullPath);
    StrLen_t iLenRelativePath = StrT::Len(pszRelativePath);
    if (iLenRelativePath > iLenFullPath) return false;
    if (!ComparePath(pszFullPath + (iLenFullPath - iLenRelativePath), pszRelativePath, iLenRelativePath)) return true;
    return false;
}

cFilePath GRAYCALL cFilePath::MakeRelativePath(const FILECHAR_t* pszFullPath, const FILECHAR_t* pszRootDir) {  // static
    //! given a pszFullPath, subtract pszRootDir out to make a relative path.
    //! If pszFullPath is not relative to pszRootDir just return the pszFullPath.
    //! Try using IsRelativeRoot() before call to this.
    //! e.g. pszFullPath = "c:\data\a\b\c.txt", pszRootDir = "c:\data", Return= "a\b\c.txt"

    if (StrT::IsNullOrEmpty(pszRootDir))  return pszFullPath; // nothing here.

    StrLen_t iLen;

    // pszDir may be relative to the current path.
    FILECHAR_t szWorkDir[_MAX_PATH];
    if (IsFilePathRooted(pszRootDir)) {
        // pszRootDir is absolute path
    } else {
        // pszRootDir isn't absolute path. make it one.
        iLen = StrT::Len(pszRootDir);
        if (!ComparePath(pszFullPath, pszRootDir, iLen)) {  // shortcut test.
            // yes pszFullPath is in pszRootDir. take a shortcut.
            if (IsCharDirSep(pszFullPath[iLen])) iLen++;
            return (pszFullPath + iLen);
        }
        // So try the Current Working Directory full path next!
        MakeFullPath2(TOSPAN(szWorkDir), pszRootDir);
        pszRootDir = szWorkDir;
    }

    // IsRelativeRoot
    iLen = StrT::Len(pszRootDir);
    if (!ComparePath(pszFullPath, pszRootDir, iLen)) {
        // Skip DirSep
        pszFullPath += iLen;
        if (IsCharDirSep(pszFullPath[0])) pszFullPath++;
    } else {
        // pszFullPath is not relative path for pszRootDir. just return full path. or its already a relative path ?
    }

    return pszFullPath;
}

FILECHAR_t* GRAYCALL cFilePath::SkipRelativePrefix(const FILECHAR_t* pszName) {  // static
    //! if it has a relative prefix (.\) then skip it. it means nothing.
    while (pszName[0] == '.' && IsCharDirSep(pszName[1])) {
        pszName += 2;
    }
    return const_cast<FILECHAR_t*>(pszName);
}

FILECHAR_t* GRAYCALL cFilePath::GetFilePathUpDir2(const FILECHAR_t* pszName, StrLen_t iLen /*= k_StrLen_UNK*/, int iQtyDirs /*= 1*/) {  // static
    //! go up this many folders if possible.
    //! iQtyDirs = 1 = the folder for "sdf:/dir1/dir2/dir3/dir4" = "/dir4"
    //!  2 = for "sdf:/dir1/dir2/dir3/dir4" = "/dir3/dir4"
    //!  -1 = for "sdf:/dir1/dir2/dir3/dir4" = "/dir2/dir3/dir4"
    //! ASSUME: trailing \ is a separate directory. weird.
    //! @return
    //!  pointer to char after the DirSep or 0th.
    //!  nullptr = cant go that far.

    if (pszName == nullptr) {
        return nullptr;
    }
    if (iQtyDirs == 0) {
        return const_cast<FILECHAR_t*>(pszName);
    }

    if (iQtyDirs < 0) {
        // go up from the bottom.
        if (iLen < 0) {
            iLen = _MAX_PATH;
        }
        // Device name doesn't count.
        StrLen_t i = GetFilePathDeviceLen(pszName);
        for (; i < iLen; i++) {
            FILECHAR_t ch = pszName[i];
            if (ch == '\0') break;
            if (IsCharDirSep(ch)) {
                if (++iQtyDirs >= 0) {
                    return const_cast<FILECHAR_t*>(pszName + i + 1);
                }
            }
        }
        return nullptr;
    }

    if (iLen < 0) {
        iLen = StrT::Len(pszName);
    }

    const FILECHAR_t* pszAct = pszName + iLen - 1;
    for (; pszAct >= pszName; pszAct--) {
        if (IsCharDirSep(pszAct[0])) {
            if (--iQtyDirs <= 0) {
                pszAct++;
                return const_cast<FILECHAR_t*>(pszAct);
            }
        }
    }

    if (iQtyDirs) {
        // cant go below root !
        if (IsFilePathRooted(pszName)) return nullptr;
        if (iQtyDirs > 1) return nullptr;
    }
    return const_cast<FILECHAR_t*>(pszName);
}

cStringF GRAYCALL cFilePath::GetFilePathUpDir1(const FILECHAR_t* pszPath, StrLen_t iLen /*= k_StrLen_UNK*/, int iQtyDirs /*= 1*/) {  // static
    //! Go up a single dir. My immediate parent.
    //! @arg iQtyDirs = -1 = for "sdf:/dir1/dir2/dir3/dir4" = "sdf:/dir1/dir2/dir3"
    const FILECHAR_t* pszAct = GetFilePathUpDir2(pszPath, iLen, iQtyDirs);
    if (pszAct == nullptr) return "";
    return ToSpan(pszPath, StrT::Diff(pszAct, pszPath) - 1);
}

bool GRAYCALL cFilePath::MakeFilePathUpDir(FILECHAR_t* pszPath) {  // static
    //! Get the file path if the file were up one directory. in its parent dir.
    //! Like using ExtractDir()

    FILECHAR_t* pszTitle = GetFileName(pszPath);
    if (pszTitle == nullptr) return false;
    FILECHAR_t* pszAct = GetFilePathUpDir2(pszPath, StrT::Diff(pszTitle, pszPath), 2);
    if (pszAct == nullptr) return false;
    cMem::CopyOverlap(pszAct, pszTitle, (StrT::Len(pszTitle) + 1) * sizeof(FILECHAR_t));  // restore file name/title + '\0'
    return true;
}

StrLen_t GRAYCALL cFilePath::MakeProperPath(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFileInp, FILECHAR_t chSep) {  // static
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

    if (pszFileInp == nullptr) pszFileInp = ret;  // in place.

    bool bCopy = (ret != pszFileInp);  // in place fix is OK.

    int iFolders = 0;
    StrLen_t iOut = 0;

    while (iOut < ret.get_MaxLen()) {
        FILECHAR_t ch = *(pszFileInp++);
        FILECHAR_t chNew = ch;
        if (ch == '.') {
            StrLen_t iCountDots = 0;
            while (pszFileInp[iCountDots] == '.') iCountDots++;
            if (IsCharDirSep(pszFileInp[iCountDots])) {  // ../stuff
                // Back up a directory if we can.
                iCountDots++;
                FILECHAR_t* pszUpDir = GetFilePathUpDir2(ret.get_DataWork(), iOut, iCountDots);
                if (pszUpDir == nullptr) {  // iCountDots > iFolders
                    // Just preserve it since it cant be resolved?
                    iCountDots--;
                    if (bCopy) {
                        iOut += StrT::CopyLen(ret.get_DataWork() + iOut, pszFileInp - 1, cValT::Min<StrLen_t>(iCountDots + 2, ret.get_MaxLen() - iOut));
                    } else {
                        iOut += iCountDots + 1;
                    }
                } else {
                    iOut = StrT::Diff(pszUpDir, ret.get_DataConst());
                    bCopy = true;
                }
                pszFileInp += iCountDots;  // skip over it.
                continue;
            }
        }

        if (IsCharDirSep(ch)) {
            iFolders++;
            chNew = chSep;
        }
        if (bCopy || chNew != ch) {
            ret.get_DataWork()[iOut] = chNew;
        }
        if (ch == '\0')  // done.
            break;
        iOut++;
    }
    return iOut;
}

cFilePath GRAYCALL cFilePath::MakeProperPathX(const FILECHAR_t* pszFileInp, FILECHAR_t chSep) {  // static
    //! Make sure all forward/back slashes are same chSep.
    FILECHAR_t szFilePath[_MAX_PATH];
    MakeProperPath(TOSPAN(szFilePath), pszFileInp, chSep);
    return szFilePath;
}

StrLen_t GRAYCALL cFilePath::ExtractDir(OUT FILECHAR_t* pszFilePath, StrLen_t iLen, bool bTrailingSep) {  // static
    //! Remove the file name from this and just leave the path.
    //! bTrailingSep = leave the trailing /
    //! opposite of GetFileName()
    //! @return
    //!  length of the new string.

    FILECHAR_t* pszTitle = GetFileName(pszFilePath, iLen);
    if (pszTitle == nullptr) return 0;

    for (; pszTitle > pszFilePath; pszTitle--) {
        if (*pszTitle == ':') {  // has a drive letter.
            pszTitle++;
            break;
        }
        if (IsCharDirSep(*pszTitle)) {  // Might be LINUX
            if (bTrailingSep) pszTitle++;
            break;
        }
    }
    *pszTitle = '\0';
    return StrT::Diff(pszTitle, pszFilePath);
}

StrLen_t GRAYCALL cFilePath::ExtractDirCopy(cSpanX<FILECHAR_t>& ret, const FILECHAR_t* pszFilePathSrc, bool bTrailingSep) {  // static
    //! Remove the file name from this and just leave the path. (was ExtractDir)
    //! @arg bTrailingSep = leave the trailing /
    //! @return
    //!  length of the new string.
    const StrLen_t iLen = StrT::Copy(ret, pszFilePathSrc);
    return ExtractDir(ret.get_DataWork(), iLen, bTrailingSep);
}

cFilePath GRAYCALL cFilePath::GetFileDir(const FILECHAR_t* pszPath, bool bTrailingSep) {  // static
    //! Extract the directory from a file path. include the trailing / k_DirSep if bTrailingSep is set.
    //! @arg bTrailingSep = leave the trailing /
    FILECHAR_t szPath[_MAX_PATH];
    const StrLen_t iLen = ExtractDirCopy(TOSPAN(szPath), pszPath, bTrailingSep);
    UNREFERENCED_PARAMETER(iLen);
    return szPath;
}

bool GRAYCALL cFilePath::IsFileNameExt(const FILECHAR_t* pszFileName, const FILECHAR_t* pszExt) noexcept {  // static
    //! Is this the extension for the file name ? with or without dot. test MIME type.
    if (pszExt == nullptr) return false;
    StrLen_t iLenExt = StrT::Len(pszExt);
    StrLen_t iLenName = StrT::Len(pszFileName);
    if (iLenExt >= iLenName)  // can't be. it wouldn't fit.
    {
        return false;
    }
    return !StrT::CmpIN(pszFileName + (iLenName - iLenExt), pszExt, iLenExt);  // EndsWith
}

cFilePath GRAYCALL cFilePath::GetNameExtStar(const FILECHAR_t* pszFilePath) {  // static
    //! Convert a name possibly with a full path to a name and extension wildcard.
    //!  "dir/Name.ext"
    //! @return
    //!  "Name.*"
    static const FILECHAR_t* k_pszExt = _FN(".*");
    return ReplaceFileExt(GetFileName(pszFilePath), k_pszExt);
}

#ifdef _WIN32
const wchar_t* GRAYCALL cFilePath::MakeFileNameLongW(const FILECHAR_t* pszFilePath) {  // static
    //! Add _WIN32 k_NamePrefix if the filename is too long for the system call.
    //! Convert UTF8 to UNICODE if necessary.

    if (StrT::StartsWithI<FILECHAR_t>(pszFilePath, k_NamePrefix))  // already prefixed. So do nothing.
        return StrArg<wchar_t>(pszFilePath);

    cStringF sPathNew = k_NamePrefix;
    sPathNew += pszFilePath;
    return StrArg<wchar_t>(sPathNew);  //  UTF8 to UNICODE if necessary.
}
const wchar_t* GRAYCALL cFilePath::GetFileNameLongW(cStringF sFilePath) {  // static
    //! Add _WIN32 k_NamePrefix if the filename is too long for the system call.
    //! Convert UTF8 to UNICODE if necessary.

    if (sFilePath.GetLength() <= _MAX_PATH)  // short names don't need this.
        return StrArg<wchar_t>(sFilePath.get_CPtr());
    return MakeFileNameLongW(sFilePath);
}

const wchar_t* GRAYCALL cFilePath::GetFileNameLongW(const FILECHAR_t* pszFilePath) {  // static
    if (StrT::Len(pszFilePath) <= _MAX_PATH)                                          // short names don't need this.
        return StrArg<wchar_t>(pszFilePath);
    return MakeFileNameLongW(pszFilePath);
}
#endif
}  // namespace Gray
