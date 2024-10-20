//! @file cFileStatus.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cFile.h"
#include "cFileStatus.h"

#ifdef __linux__
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <unistd.h>
#endif

namespace Gray {
cFileStatus::cFileStatus() noexcept {
    DEBUG_CHECK(!isFileValid());
}
cFileStatus::cFileStatus(const FILECHAR_t* pszFilePath) {
    //! @note use isFileValid() to find if this is valid.
    ASSERT(!isFileValid());
    ReadFileStatus(pszFilePath);
}

void cFileStatus::InitFileStatus() noexcept {
    _timeCreate.InitTime();
    _timeChange.InitTime();  // All OS support this.
    _timeLastAccess.InitTime();
    _nSize = CastN(FILE_SIZE_t, -1);  // Set to an invalid value.
    _AttributeFlags = FILEATTR_t::_None;
    DEBUG_CHECK(!isFileValid());
}

void cFileStatus::InitFileStatus(const cFileStatusSys& statusSys) {
    //! convert from OS native format.
#ifdef _WIN32
    _timeCreate = statusSys.ftCreationTime;  // cTimeFile
    _timeChange = statusSys.ftLastWriteTime;
    _timeLastAccess = statusSys.ftLastAccessTime;
    _nSize = statusSys.nFileSizeLow | ((FILE_SIZE_t)statusSys.nFileSizeHigh) << 32;
    _AttributeFlags = CastN(FILEATTR_t, statusSys.dwFileAttributes);  // truncated!
#elif defined(__linux__)
    // http://linux.die.net/man/2/stat
    // hidden file start with .
    _timeCreate = cTimeInt(statusSys.st_ctime).GetAsFileTime();  // time_t
    _timeChange = cTimeInt(statusSys.st_mtime).GetAsFileTime();
    _timeLastAccess = cTimeInt(statusSys.st_atime).GetAsFileTime();
    _nSize = statusSys.st_size;
    _AttributeFlags = FILEATTR_t::_None;  // check the read,write,execute bits?
    if (S_ISREG(statusSys.st_mode)) {
        _AttributeFlags.SetMask(FILEATTR_t::_Normal);
    } else if (S_ISDIR(statusSys.st_mode)) {
        _AttributeFlags.SetMask(FILEATTR_t::_Directory);
    } else if (S_ISLNK(statusSys.st_mode)) {
        _AttributeFlags.SetMask(FILEATTR_t::_Link);
    } else {                                        // S_ISBLK, S_ISSOCK, S_ISCHR, S_ISFIFO
        _AttributeFlags.SetMask(FILEATTR_t::_Volume);  // device of some sort.
    }
#endif
}

#if defined(__linux__)
HRESULT GRAYCALL cFileStatus::GetStatusSys(OUT cFileStatusSys& statusSys, const FILECHAR_t* pszName, bool bFollowLinks) {  // static
    // https://man7.org/linux/man-pages/man2/stat.2.html
    int iRet = (bFollowLinks) ? ::lstat(pszName, &statusSys) : ::stat(pszName, &statusSys);
    if (iRet != 0) {
        return HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
    }
    return S_OK;
}
#endif

HRESULT GRAYCALL cFileStatus::WriteFileAttributes(const FILECHAR_t* pszFilePath, FILEATTR_t dwAttributes) {  // static
    //! Set attributes for a NON open file.
    //! dwAttributes = FILEATTR_t::_Hidden, FILEATTR_t::_ReadOnly
#ifdef _WIN32
    if (!::SetFileAttributesW(cFilePath::GetFileNameLongW(pszFilePath), static_cast<UINT32>(dwAttributes))) {
        return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
    }
#elif 0  // defined(__linux__)
         // TODO __linux__ fchmod() to set file permissions.
         // convert FILEATTR_t to Linux bits
    ASSERT(0);
    if (::chmod(pszFilePath, dwAttributes) != 0) {
        return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
    }
#endif
    return S_OK;
}

HRESULT GRAYCALL cFileStatus::WriteFileTimes(const FILECHAR_t* pszFilePath, const cTimeFile* pTimeCreate, const cTimeFile* pTimeChange) {  // static
    //! Update the created/changed time for a file. (by name) (similar to 'touch' command)
    //! May have varying levels of support for OS, FAT, NTFS, NFS, etc
    cFile file;
    HRESULT hRes = file.OpenX(pszFilePath, OF_READWRITE | OF_BINARY);  // OPEN_EXISTING
    if (FAILED(hRes)) return hRes;

    bool bRet = file.SetFileTime(pTimeCreate, nullptr, pTimeChange);
    if (!bRet) return HResult::GetLastDef(E_HANDLE);

    return S_OK;
}

HRESULT GRAYCALL cFileStatus::WriteFileTimes(const FILECHAR_t* pszFilePath, const cFileStatus& rFileStatus) {  // static
    return WriteFileTimes(pszFilePath, &(rFileStatus._timeCreate), &(rFileStatus._timeChange));
}

HRESULT GRAYCALL cFileStatus::ReadFileStatus2(const FILECHAR_t* pszFilePath, cFileStatus* pFileStatus, bool bFollowLink) {  // static
#ifdef _WIN32
    // NOTE: same as _WIN32 GetFileAttributesEx()
    ::WIN32_FIND_DATAW statusSys;  // cFileStatusSys
    ::HANDLE hContext = ::FindFirstFileW(cFilePath::GetFileNameLongW(pszFilePath), &statusSys);
    if (hContext == INVALID_HANDLE_VALUE) {
        // DEBUG_ERR(( "Can't open input dir [%s]", LOGSTR(pszFilePath) ));
        return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
    }

    // Was this a link ? FILEATTR_t::_Link
    if (bFollowLink && (statusSys.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
    }

    ::FindClose(hContext);

#elif defined(__linux__)

    cFileStatusSys statusSys;
    HRESULT hRes = cFileStatus::GetStatusSys(statusSys, pszFilePath, bFollowLink);
    if (FAILED(hRes)) return hRes;
#endif

    if (pFileStatus != nullptr) {
        pFileStatus->InitFileStatus(statusSys);
        pFileStatus->UpdateLinuxHidden(cFilePath::GetFileName(StrT::ToSpanStr(pszFilePath)));
        ASSERT(pFileStatus->isFileValid());
    }

    return S_OK;  // file exists.
}
}  // namespace Gray
