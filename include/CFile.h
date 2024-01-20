//
//! @file cFile.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cFile_H
#define _INC_cFile_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cException.h"
#include "cFilePath.h"
#include "cFileStatus.h"
#include "cOSHandle.h"
#include "cObject.h"
#include "cStream.h"
#include "cTimeInt.h"

#if defined(__linux__)
// #include <sys/types.h>
#include <fcntl.h>  // O_RDONLY
// #include <unistd.h>
struct _SECURITY_ATTRIBUTES;  // stub this out
#endif

namespace Gray {
typedef UINT32 OF_FLAGS_t;
/// <summary>
/// enumeration of file open mode/control flags. cBitmask
/// </summary>
enum OF_FLAGS_ENUM_t : OF_FLAGS_t {
#if defined(__linux__)
    OF_READ = O_RDONLY,     /// _O_RDONLY
    OF_WRITE = O_WRONLY,    /// _O_WRONLY
    OF_READWRITE = O_RDWR,  /// _O_RDWR
    OF_APPEND = O_APPEND,   /// writes starting at EOF
    OF_CREATE = O_CREAT,    /// _O_CREAT = create the file if it doesn't exist. overwrite it if it does.

    OF_SHARE_COMPAT = 0x00000000,      /// no Linux function.
    OF_SHARE_EXCLUSIVE = 0x00000000,   /// no Linux function. O_EXCL
    OF_SHARE_DENY_WRITE = 0x00000000,  /// no Linux function.
    OF_SHARE_DENY_READ = 0x00000000,   /// no Linux function.
    OF_SHARE_DENY_NONE = 0x00000000,   /// no Linux function.

    OF_EXIST = 0x00000000,  /// just test if it exists. like _access()
#endif

#if defined(UNDER_CE)
    OF_READ = 0x0000,       /// _O_RDONLY
    OF_WRITE = 0x0001,      /// _O_WRONLY
    OF_READWRITE = 0x0002,  /// _O_RDWR
    OF_APPEND = 0x0008,     /// O_APPEND writes done at EOF
    OF_CREATE = 0x0100,     /// _O_CREAT = create the file if it doesn't exist. overwrite it if it does.

    OF_SHARE_COMPAT = 0x00000000,
    OF_SHARE_EXCLUSIVE = 0x00000010,   /// O_EXCL
    OF_SHARE_DENY_WRITE = 0x00000020,  /// not defined in __linux__ O_EXCL
    OF_SHARE_DENY_READ = 0x00000030,
    OF_SHARE_DENY_NONE = 0x00000040,

    OF_EXIST = 0x00004000,  /// just test if it exists. like _access()
#endif

    //! High flags not supported by POSIX open().
    OF_OPEN_MASK = 0x00FFFFFF,  /// flags used by open().
    // OF_CACHE_RAND		= 0x04000000,
    OF_CACHE_SEQ = 0x08000000,  /// O_DIRECT for __linux__ ??
    OF_BINARY = 0x10000000,     /// for using FILE* in non text mode. (Default)
    OF_TEXT = 0x20000000,       /// UTF8 or plain ASCII text file. (set by use of char Read/WriteString functions)
    OF_NONCRIT = 0x40000000,    /// Not a real failure if it doesn't exist.
                                // OF_TEXT_W		= 0x80000000,	/// UNICODE text file. (set by use of wchar_t Read/WriteString functions)
};

struct GRAYCORE_LINK cFileStatus;

/// <summary>
/// Wrapper for General OS file access interface. Extends MFC functionality
/// @note Any file can be a cStreamOutput of text as well.
/// Dupe the MFC functionality we need from CFile. Similar to IStream and CAtlFile
/// </summary>
class GRAYCORE_LINK cFile : public cObject, public cOSHandle, public cStream {
 protected:
    static ITERATE_t sm_iFilesOpen;                  /// global count of all open files for this process.
    OF_FLAGS_t m_nOpenFlags = CastN(OF_FLAGS_t, 0);  /// cBitmask MMSYSTEM uses high bits of 32 bit flags. OF_FLAGS_t OF_READ etc
    cFilePath m_strFileName;                         /// store a copy of the full file path. MFC defined name.

 protected:
    HRESULT OpenSetup(cFilePath sFilePath, OF_FLAGS_t uModeFlags);
    HRESULT OpenCreate2(cFilePath sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = CastN(OF_FLAGS_t, OF_CREATE | OF_WRITE), ::_SECURITY_ATTRIBUTES* pSa = nullptr);

 public:
    cFile() noexcept : m_nOpenFlags(CastN(OF_FLAGS_t, 0)) {}
    cFile(cStringF sFilePath, OF_FLAGS_t nOpenFlags) {
        OpenX(sFilePath, nOpenFlags);
    }
    ~cFile() override {
        Close();
    }

    bool isValidCheck() const noexcept override {  /// memory allocation and structure definitions are valid.
        if (!cObject::isValidCheck()) return false;
        if (!IsValidCast<const cFile>(this))  // structure definitions are valid..
            return false;
        return true;
    }

    /// <summary>
    /// Get full file path.
    /// like _MFC_VER CFile::GetFilePath(); but cStringF.
    /// @note Don't use GetFilePath() as it has some weird side effects in MFC.
    /// like WIN32 GetFinalPathNameByHandle().
    /// __linux__ readlink on /proc/self/fd/NNN where NNN is the file descriptor.
    /// </summary>
    cFilePath get_FilePath() const noexcept {
        return m_strFileName;
    }

    /// <summary>
    /// Get file name and ext. No Path. Not the same as "title" which has no ext.
    /// </summary>
    const FILECHAR_t* get_FileName() const;

    const FILECHAR_t* get_FileExt() const;
    bool IsFileNameExt(const FILECHAR_t* pszExt) const noexcept;

    // File Mode stuff.
    OF_FLAGS_t get_Mode() const noexcept {
        //! get basic set of OF_FLAGS_t. get rid of OF_NONCRIT type flags. e.g. OF_READ
        return m_nOpenFlags & OF_OPEN_MASK;  //
    }
    OF_FLAGS_t get_ModeFlags() const noexcept {
        //! Get the full/hidden elements of the OF_FLAGS_t Flags. e.g. OF_NONCRIT
        return m_nOpenFlags;
    }
    bool isModeWrite() const noexcept {
        // Can i write ?
        const OF_FLAGS_t nFlagsDir = (m_nOpenFlags & (OF_WRITE | OF_READ | OF_READWRITE));
        return nFlagsDir == OF_WRITE || nFlagsDir == OF_READWRITE;
    }
    bool isModeRead() const noexcept {
        // Can i read ?
        const OF_FLAGS_t nFlagsDir = (m_nOpenFlags & (OF_WRITE | OF_READ | OF_READWRITE));
        return nFlagsDir == OF_READ || nFlagsDir == OF_READWRITE;  // assume OF_READ = 0
    }

    // File Open/Close
#if defined(__linux__)
    HRESULT GetStatusSys(OUT cFileStatusSys& statusSys);
#endif

    // MFC Open is BOOL return type.
    HRESULT OpenCreate(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_CREATE | OF_WRITE, ::_SECURITY_ATTRIBUTES* pSa = nullptr);
    virtual HRESULT OpenX(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE);
    virtual void Close() noexcept;

    HANDLE DetachHandle() noexcept;

    HRESULT OpenWait(cStringF sFilePath = _FN(""), OF_FLAGS_t nOpenFlags = OF_READ | OF_SHARE_DENY_NONE, TIMESYSD_t nWaitTime = 100);
    virtual HRESULT SetLength(STREAM_POS_t dwNewLen);

    // File Access
    bool SetFileTime(const cTimeFile* lpCreationTime, const cTimeFile* lpAccessTime, const cTimeFile* lpLastWriteTime);
    bool SetFileTime(cTimeInt timeCreation, cTimeInt timeLastWrite);
    HRESULT GetFileStatus(OUT cFileStatus& attr) const;

    // cStream override
    HRESULT ReadX(void* pData, size_t nDataSize) noexcept override;
    HRESULT WriteX(const void* pData, size_t nDataSize) override;  // disambiguate.
    HRESULT FlushX() override;
    STREAM_POS_t GetPosition() const override;
    STREAM_POS_t GetLength() const override;

    HRESULT SeekX(STREAM_OFFSET_t nOffset, SEEK_t eSeekOrigin = SEEK_t::_Set) noexcept override {  // disambig
        return cOSHandle::SeekX(nOffset, eSeekOrigin);
    }

    static HRESULT GRAYCALL DeletePath(const FILECHAR_t* pszFileName);  // NOTE: MFC Remove() returns void
    static HRESULT GRAYCALL DeletePathX(const FILECHAR_t* pszFilePath, FILEOPF_t nFileFlags = FILEOPF_t::_None);
    static HRESULT GRAYCALL LoadFile(const FILECHAR_t* pszFilePath, OUT cBlob& blob, size_t nSizeExtra = 0);
};
}  // namespace Gray
#endif  // _INC_cFile_H
