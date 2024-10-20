//! @file cFileStatus.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cFileStatus_H
#define _INC_cFileStatus_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "HResult.h"
#include "cStreamProgress.h"
#include "cTimeFile.h"
#include "cTimeInt.h"
#include "cValSpan.h"
#include "cBits.h"

#ifdef _WIN32
typedef WIN32_FIND_DATAW cFileStatusSys;  // or BY_HANDLE_FILE_INFORMATION ?
#else
struct stat;
typedef struct stat cFileStatusSys;  // from stat(), lstat() or fstat()
#endif

namespace Gray {
#ifdef _WIN32
typedef ULONGLONG FILE_SIZE_t;  /// similar to STREAM_POS_t size_t
#else
typedef UINT64 FILE_SIZE_t;          /// similar to STREAM_POS_t size_t
#endif

/// <summary>
/// cBitmask FAT, FAT32 and NTFS file attribute flags. translated to approximate __linux__ NFS
/// </summary>
enum class FILEATTR_t : UINT32 {
    _None = 0,
    _ReadOnly = 0x0001,  /// FILE_ATTRIBUTE_READONLY. __linux__ permissions for user ?
    _Hidden = 0x0002,    /// FILE_ATTRIBUTE_HIDDEN. __linux__ starts with .
    _System = 0x0004,    /// FILE_ATTRIBUTE_SYSTEM
    _SystemMask = 0x0106, // Hidden, System, Temporary 

    _Directory = 0x0010,  /// FILE_ATTRIBUTE_DIRECTORY
    _Archive = 0x0020,    /// FILE_ATTRIBUTE_ARCHIVE = this has been changed. (needs to be archived) not yet backed up.
    _Volume = 0x0040,     /// FILE_ATTRIBUTE_DEVICE = some sort of device. not a file or dir. e.g. COM1

    _Normal = 0x0080,     /// FILE_ATTRIBUTE_NORMAL = just a file.
    _NormalMask = 0x0087,  /// (FILEATTR_t::_ReadOnly|FILEATTR_t::_FILEATTR_Hidden|FILEATTR_t::_FILEATTR_System)

    // NTFS only flags. (maybe Linux)
    _Temporary = 0x0100,  /// FILE_ATTRIBUTE_TEMPORARY
    _Link = 0x0400,       /// FILE_ATTRIBUTE_REPARSE_POINT = a link. This file doesn't really exist locally but is listed in the directory anyhow.
    _Compress = 0x0800,   /// FILE_ATTRIBUTE_COMPRESSED. this is a file that will act like a ATTR_directory. (sort of)
};
 
/// <summary>
/// Attributes for a file (or directory) in a directory. Does NOT store the name.
/// Support of fields varies based on file system. FAT,FAT32,NTFS,NFS etc
/// Similar to ANSI (or POSIX) stat() _stat
/// Similar to MFC cFileStatus
/// </summary>
struct GRAYCORE_LINK cFileStatus {
    typedef cFileStatus THIS_t;

    cTimeFile _timeChange;                          /// m_mtime = real world time/date of last modification. (FAT32 only accurate to 2 seconds) // All OS support this.
    cTimeFile _timeCreate;                          /// m_ctime  = (may not be supported by file system).
    cTimeFile _timeLastAccess;    /// m_atime = time of last access/Open. (For Caching). (may not be supported by file system)
    FILE_SIZE_t _nSize = CastN(FILE_SIZE_t,-1);     /// file size in bytes. size_t. not always accurate for directories. (-1 = invalid size)
    cBitmask<FILEATTR_t, UINT32> _AttributeFlags = FILEATTR_t::_None;  /// cBitmask of file attribute bits. FILEATTR_None

    cFileStatus() noexcept;
    cFileStatus(const FILECHAR_t* pszFilePath);

    void InitFileStatus() noexcept;
    void InitFileStatus(const cFileStatusSys& statusSys);

#if defined(__linux__)
    static HRESULT GRAYCALL GetStatusSys(OUT cFileStatusSys& statusSys, const FILECHAR_t* pszName, bool bFollowLinks = false);
#endif

    static bool IsLinuxHidden(const FILECHAR_t* pszName) noexcept {
        //! Is this a hidden file on __linux__ (NFS) ? all files starting with dot.
        if (pszName == nullptr) return true;
        return pszName[0] == '.';
    }
    bool UpdateLinuxHidden(const FILECHAR_t* pszName) noexcept {
        //! Is this a __linux__ (NFS) hidden file name ? starts with dot.
        if (IsLinuxHidden(pszName)) {
            _AttributeFlags.SetMask(FILEATTR_t::_Hidden);
            return true;
        }
        return false;
    }

    /// <summary>
    /// did i get file data? is this a file vs a device?
    /// @note asking for a 'devicename' is BAD! i.e. http://myserver/com5.txt (this will trap that!)
    /// </summary>
    /// <returns>false = bad (or not a) file like 'com1:' 'lpt:' etc.</returns>
    bool isFileValid() const noexcept {
        return _timeChange.isValid();
    }

    static COMPARE_t GRAYCALL CompareChangeFileTime(const cTimeFile& t1, const cTimeFile& t2) noexcept {  //! (accurate to 2 seconds)
        //! ~2 sec accurate for FAT32
        return cValT::Compare(t1.get_FAT32(), t2.get_FAT32());
    }
    bool IsSameChangeFileTime(const cTimeFile& t2) const noexcept {  //! (accurate to 2 seconds)
        //! ~2 sec accurate for FAT32
        return cValT::Compare(_timeChange.get_FAT32(), t2.get_FAT32()) == COMPARE_Equal;
    }
    static constexpr TIMESEC_t MakeFatTime(TIMESEC_t tTime) noexcept {
        //! (accurate to 2 seconds)
        return tTime & ~1;
    }
    /// <summary>
    /// ~2 second accurate for FAT32
    /// </summary>
    static COMPARE_t GRAYCALL CompareChangeTime(const cTimeInt& t1, const cTimeInt& t2) noexcept {
        return cValT::Compare(MakeFatTime(t1.GetTime()), MakeFatTime(t2.GetTime()));
    }
    bool IsSameChangeTime(const cTimeInt& t2) const noexcept {
        return CompareChangeTime(_timeChange, t2) == COMPARE_Equal;
    }

    bool IsFileEqualTo(const THIS_t& rFileStatus) const noexcept {
        if (cValT::Compare(_timeCreate.get_Val(), rFileStatus._timeCreate.get_Val()) != COMPARE_Equal) return false;
        if (!IsSameChangeFileTime(rFileStatus._timeChange)) return false;
        if (_nSize != rFileStatus._nSize) return false;
        return true;
    }
    bool IsFileEqualTo(const THIS_t* pFileStatus) const noexcept {
        //! do these 2 files have the same attributes.
        if (pFileStatus == nullptr) return false;
        return IsFileEqualTo(*pFileStatus);
    }
    /// <summary>
    /// have this attribute? e.g. FILEATTR_t::_ReadOnly
    /// </summary>
    bool IsAttrMask(FILEATTR_t dwAttrMask = FILEATTR_t::_ReadOnly) const noexcept {
        return _AttributeFlags.HasAny(dwAttrMask);
    }
    bool isAttrDir() const noexcept {
        return IsAttrMask(FILEATTR_t::_Directory);
    }
    bool isAttrHidden() const noexcept {
        // for __linux__ starts with .
        return IsAttrMask(FILEATTR_t::_Hidden);
    }
    /// <summary>
    /// get the 64 bit length of the file.
    /// </summary>
    /// <returns>-1 = size not available for directories.</returns>
    FILE_SIZE_t GetFileLength() const noexcept {
        return _nSize;
    }

    static HRESULT GRAYCALL WriteFileAttributes(const FILECHAR_t* pszFilePath, FILEATTR_t dwAttributes);
    static HRESULT GRAYCALL WriteFileTimes(const FILECHAR_t* pszFilePath, const cTimeFile* pTimeCreate, const cTimeFile* pTimeChange = nullptr);
    static HRESULT GRAYCALL WriteFileTimes(const FILECHAR_t* pszFilePath, const cFileStatus& rFileStatus);

    /// <summary>
    /// get info/attributes/status on a single file or dir. like OF_EXIST ?
    /// Similar to the MFC CFileFind. Are wildcards allowed ??
    /// @note cFilePath::IsFilePathRoot will fail.
    /// </summary>
    /// <param name="pszFilePath"></param>
    /// <param name="pFileStatus">is allowed to be nullptr</param>
    /// <param name="bFollowLink"></param>
    /// <returns>S_OK</returns>
    static HRESULT GRAYCALL ReadFileStatus2(const FILECHAR_t* pszFilePath, cFileStatus* pFileStatus = nullptr, bool bFollowLink = false);

    static bool GRAYCALL Exists(const FILECHAR_t* pszFilePath) {
        //! boolean true if this file exists? I can read it. Does not mean I can write to it. like OF_EXISTS
        const HRESULT hRes = ReadFileStatus2(pszFilePath, nullptr, true);
        return SUCCEEDED(hRes);
    }

    HRESULT ReadFileStatus(const FILECHAR_t* pszFilePath, bool bFollowLink = false) {
        return ReadFileStatus2(pszFilePath, this, bFollowLink);
    }
};
}  // namespace Gray
#endif  // _INC_cFileStatus_H
