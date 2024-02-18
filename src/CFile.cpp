//! @file cFile.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "PtrCast.h"
#include "cFile.h"
#include "cFileDir.h"
#include "cLogMgr.h"
#include "cString.h"
#include "cTimeSys.h"

#ifdef __linux__
#include "cTimeVal.h"
#include <sys/stat.h>
#include <sys/time.h>
#else
#include <shellapi.h>  // FOF_RENAMEONCOLLISION
#endif

namespace Gray {
ITERATE_t cFile::sm_iFilesOpen = 0;  // debug statistical info

HRESULT cFile::OpenSetup(cFilePath sFilePath, OF_FLAGS_t nOpenFlags) {
    //! Internal function to set internal params.
    //! Similar to SetFilePath() in MFC ?
    //! @arg nOpenFlags = OF_BINARY | OF_WRITE
    //! @return
    //!  S_FALSE = success , already open.
    //!  S_OK = success

    if (sFilePath.IsEmpty() || get_FilePath().IsEqualNoCase(sFilePath)) {
        // Name is already set ? and already open ?
        if (isValidHandle() && m_nOpenFlags == nOpenFlags) {
            ASSERT(!get_FilePath().IsEmpty());
            return S_FALSE;
        }
    }

    Close();  // Make sure it's closed first. re-using this structure.

    if (!sFilePath.IsEmpty()) {
        m_strFileName = sFilePath;
    }

    m_nOpenFlags = nOpenFlags;

    // DEBUG_TRACE(( "Open file '%s' flags=0%x", LOGSTR(m_strFileName), nOpenFlags ));
    return S_OK;
}

HRESULT cFile::OpenCreate2(cFilePath sFilePath, OF_FLAGS_t nOpenFlags, ::_SECURITY_ATTRIBUTES* pSa) {
    //! Open a file handle.
    //! Allow convert from UTF8 to UNICODE (_WIN32) or assume native UTF8 allowed.
    //! 1. OF_EXIST = just test if this file exists, don't really open it.
    //! 2. OF_READ = open for read. file must exist.
    //! 3. OF_WRITE = open for over write. file must exist unless OF_CREATE is set.
    //! 4. OF_READWRITE = open file for write append. file must exist unless OF_CREATE is set.
    //! | OF_CREATE = create. wipe any existing file.
    //! | OF_TEXT = this is really a FILE* handle on "t" mode. else OF_BINARY
    //! | OF_CACHE_SEQ = we read this sequentially only. no seek will be used. FILE_FLAG_SEQUENTIAL_SCAN
    //! OF_READWRITE | OF_CREATE = Open for write but create if it does not already exist.

    UNREFERENCED_PARAMETER(pSa);

#ifdef _WIN32
    // NOTE: FILE_ATTRIBUTE_HIDDEN and OF_CREATE will create a hidden file!
    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;  // FILE_ATTRIBUTE_READONLY
    DWORD dwDesiredAccess = GENERIC_READ;
    switch (nOpenFlags & (OF_READ | OF_WRITE | OF_READWRITE)) {  // BEWARE OF_READ = 0
        case OF_WRITE:
            dwDesiredAccess = GENERIC_WRITE;
            break;
        case OF_READWRITE:
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
            dwFlagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;  // for open of directories.
            break;
    }

    // NOTE: FILE_SHARE_DELETE doesn't work for 98/ME
    DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;  // OF_SHARE_COMPAT|FILE_SHARE_DELETE
    switch (nOpenFlags & (OF_SHARE_COMPAT | OF_SHARE_EXCLUSIVE | OF_SHARE_DENY_WRITE | OF_SHARE_DENY_READ | OF_SHARE_DENY_NONE)) {
        case OF_SHARE_COMPAT:
        case OF_SHARE_DENY_NONE:
            dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;  // |FILE_SHARE_DELETE
            break;
        case OF_SHARE_EXCLUSIVE:
            dwShareMode = 0;
            break;
        case OF_SHARE_DENY_WRITE:
            dwShareMode = FILE_SHARE_READ;
            break;
        case OF_SHARE_DENY_READ:
            dwShareMode = FILE_SHARE_WRITE;  // |FILE_SHARE_DELETE
            break;
    }

    DWORD dwCreationDisposition = OPEN_EXISTING;  // CREATE_NEW | OPEN_EXISTING
    if (nOpenFlags & OF_CREATE) {
        // Create file with dwFlagsAndAttributes
        switch (nOpenFlags & (OF_READ | OF_WRITE | OF_READWRITE)) {
            case OF_READWRITE:
            case OF_READ:
                dwCreationDisposition = OPEN_ALWAYS;  // append if exists. create if not.
                break;
            case OF_WRITE:
                dwCreationDisposition = CREATE_ALWAYS;  // truncate if exist. create if not.
                break;
        }
    } else {
        // Named file with any attributes??
        // dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN;
    }
    if (nOpenFlags & OF_EXIST) {  // test if exists.
        dwCreationDisposition = OPEN_EXISTING;
    }
    if (nOpenFlags & OF_CACHE_SEQ) {
        dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
    }

    AttachHandle(::CreateFileW(cFilePath::GetFileNameLongW(sFilePath), dwDesiredAccess, dwShareMode, nullptr, dwCreationDisposition, dwFlagsAndAttributes, HANDLE_NULL));

#elif defined(__linux__)
    // http://linux.die.net/man/2/open
    // flags must include one of the following access modes: O_RDONLY, O_WRONLY, or O_RDWR
    // ASSUME nOpenFlags = Linux OpenFlags; O_RDONLY, O_CREAT, O_APPEND, etc.
    // O_NOFOLLOW = don't follow the symbolic link. (if a link)
    UINT uFlags = (nOpenFlags & OF_OPEN_MASK);
    UINT uMode = 0;
    if (nOpenFlags & OF_CREATE) {
        uFlags |= O_TRUNC;
        uMode = S_IRWXU | S_IRWXG;  // S_IRWXO
    }
    if (nOpenFlags & OF_CACHE_SEQ) {
        uFlags |= O_DIRECT;
    }
    m_hFile.OpenHandle(sFilePath, uFlags, uMode);
#else
#error NOOS
#endif

    if (!isValidHandle()) {
        // E_ACCESSDENIED for a hidden file?
        return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
    }
    return S_OK;
}

HRESULT cFile::OpenCreate(cStringF sFilePath, OF_FLAGS_t nOpenFlags, ::_SECURITY_ATTRIBUTES* pSa) {
    //! Open a file by name.
    //! @arg nOpenFlags = OF_READ | OF_WRITE | OF_READWRITE
    //! Expect the file pointer to be at 0!
    UNREFERENCED_PARAMETER(pSa);

    HRESULT hRes = OpenSetup(sFilePath, nOpenFlags);
    if (hRes != S_OK) {
        // hRes == S_FALSE = no change. already open.
        if (FAILED(hRes)) return hRes;
        ASSERT(isValidHandle());
        SeekToBegin();  // act like a new open()
        return S_OK;
    }

    ASSERT(!isValidHandle());
    hRes = OpenCreate2(m_strFileName, nOpenFlags, nullptr);
    if (hRes == HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND) && (nOpenFlags & OF_CREATE)) {
        // must create the stupid path first.
        hRes = cFileDir::CreateDirForFileX(m_strFileName);
        if (SUCCEEDED(hRes)) {
            hRes = OpenCreate2(m_strFileName, nOpenFlags, nullptr);
        }
    }
    if (FAILED(hRes)) return hRes;

    sm_iFilesOpen++;
    DEBUG_CHECK(sm_iFilesOpen >= 0);
    return S_OK;
}

HRESULT cFile::OpenX(cStringF sFilePath, OF_FLAGS_t nOpenFlags) {  // virtual
    return OpenCreate(sFilePath, nOpenFlags, nullptr);             // OF_TEXT
}

HRESULT cFile::OpenWait(cStringF sFilePath, OF_FLAGS_t nOpenFlags, TIMESYSD_t nTimeWait) {
    //! Try to open the file. Wait for a bit if it fails to open.
    //! If the file is locked because 'access is denied' then just wait and keep trying.

    cTimeSys tStart(cTimeSys::GetTimeNow());
    for (int iTries = 0;; iTries++) {
        HRESULT hRes = OpenX(sFilePath, nOpenFlags);  // use OpenX which is the virtual.
        if (hRes == S_OK) break;

        // failed to open
        if (hRes != HRESULT_WIN32_C(ERROR_ACCESS_DENIED)) {  // E_ACCESSDENIED
            return hRes;
        }

#ifdef _WIN32
        // try to make the file writable if marked as read only ?
        if (iTries == 0 && (nOpenFlags & OF_WRITE)) {
            hRes = cFileStatus::WriteFileAttributes(sFilePath, FILEATTR_t::_Normal);
            if (FAILED(hRes)) return hRes;
            continue;  // just try again without waiting.
        }
#endif
        if (iTries > 0 && tStart.get_AgeSys() >= nTimeWait) {
            return hRes;
        }

        // maybe its just being synced by the system? wait a bit.
        TIMESYSD_t nTimeWaitInt = cValT::Min<TIMESYSD_t>(100, nTimeWait);
        cThreadId::SleepCurrent(nTimeWaitInt);
    }
    return S_OK;
}

void cFile::Close() noexcept {  // virtual
    //! cStream
    if (!isValidHandle()) return;
    sm_iFilesOpen--;
    DEBUG_CHECK(sm_iFilesOpen >= 0);
    CloseHandle();  // NOT __super from cStream
}

HANDLE cFile::DetachHandle() noexcept {
    if (!isValidHandle()) return INVALID_HANDLE_VALUE;  //  CFile::hFileNull;
    HANDLE h = cOSHandle::DetachHandle();               // Dont do anything with this handle. assume caller takes care or it.
    sm_iFilesOpen--;
    DEBUG_CHECK(sm_iFilesOpen >= 0);
    return h;
}

const FILECHAR_t* cFile::get_FileName() const {
    return cFilePath::GetFileName(get_FilePath());
}

const FILECHAR_t* cFile::get_FileExt() const {
    //! get the EXTension including the .
    //! Must replace the stupid MFC version of this.
    return cFilePath::GetFileNameExt(get_FilePath(), get_FilePath().GetLength());
}

bool cFile::IsFileNameExt(const FILECHAR_t* pszExt) const noexcept {
    //! is the pszExt a match? MIME
    return cFilePath::IsFileNameExt(get_FilePath(), pszExt);
}

STREAM_POS_t cFile::GetPosition() const {  // virtual
    //! Get the current read position in the file.
    if (!isValidHandle()) return k_STREAM_POS_ERR;
    return SeekRaw(0, SEEK_t::_Cur);
}

#if defined(__linux__)
HRESULT CFile::GetStatusSys(OUT cFileStatusSys& statusSys) {
    // https://man7.org/linux/man-pages/man2/stat.2.html
    int iRet = ::fstat(m_hFile, &statusSys);
    if (iRet != 0) return HResult::GetPOSIXLastDef(E_HANDLE);
    return S_OK;
}
#endif

STREAM_POS_t cFile::GetLength() const {  // virtual
    //! Get the size of the open file in bytes. like MFC
    //! @return <0 = error. (or directory?)
    if (!isValidHandle()) return k_STREAM_POS_ERR;  // E_HANDLE
#ifdef _WIN32
#ifdef USE_FILE_POS64
    LARGE_INTEGER FileSize;
    bool bRet = ::GetFileSizeEx(get_Handle(), &FileSize);
    if (!bRet) return k_STREAM_POS_ERR;
    return (STREAM_POS_t)FileSize.QuadPart;
#else
    return ::GetFileSize(get_Handle(), nullptr);
#endif

#elif defined(__linux__)
    cFileStatusSys statusSys;
    HRESULT hRes = GetStatusSys(statusSys);
    if (FAILED(hRes)) return k_STREAM_POS_ERR;
    return statusSys.st_size;
#endif
}

HRESULT cFile::SetLength(STREAM_POS_t dwNewLen) {  // virtual
    //! Grow/Shrink the file.
    //! Since MFC has void return use HResult::GetLast()
    ASSERT(isValidHandle());
    HRESULT hRes = S_OK;   
#ifdef _WIN32
    SeekRaw(dwNewLen, SEEK_t::_Set);      // SetFilePointer() . maybe beyond end ?
    if (!::SetEndOfFile(get_Handle())) {  // truncate to this length
        // ASSUME HResult::GetLast() set.
        hRes = HResult::GetLast();
        DEBUG_ERR(("cFile::SetLength %d ERR='%s'", dwNewLen, LOGERR(hRes)));
        // CFileException::ThrowOsError( hRes, m_strFileName);
    } else {
        ::SetLastError(NO_ERROR);   // Why is this my job ? 
    }
#elif defined(__linux__)
    if (::ftruncate(m_hFile, dwNewLen) < 0) {
        // PosixError
        hRes = HResult::GetLast();
        DEBUG_ERR(("cFile::SetLength %d ERR='%s'", dwNewLen, LOGERR(hRes)));
    }
#endif
    return hRes;
}

bool cFile::SetFileTime(const cTimeFile* lpCreationTime, const cTimeFile* lpAccessTime, const cTimeFile* lpLastWriteTime) {
    //! Set the time access for an open file.
    //! lpAccessTime = can be null.
    //! @note lpLastWriteTime is the only time guaranteed to work on all systems.
    //! @note FAT32 lpLastWriteTime is only accurate to 2 seconds !!
    //! Similar to CFile::SetStatus() and __linux__ utime()
    //! @return true = OK;

    if (!isValidHandle()) return false;

#ifdef _WIN32
    return ::SetFileTime(get_Handle(), lpCreationTime, lpAccessTime, lpLastWriteTime);
#elif defined(__linux__)
    // NOTE: __linux__ can't set lpCreationTime?
    cTimeVal tv[2];
    if (lpAccessTime == nullptr || lpLastWriteTime == nullptr) {
        // must get defaults.
        cFileStatusSys statusSys;
        HRESULT hRes = GetStatusSys(statusSys);
        if (SUCCEEDED(hRes)) {
            tv[0].tv_sec = statusSys.st_atime;
            tv[1].tv_sec = statusSys.st_mtime;
            // statusSys.st_ctime; // ??
        }
    }
    if (lpAccessTime != nullptr) tv[0] = lpAccessTime->get_TimeVal();

    if (lpLastWriteTime != nullptr) tv[1] = lpLastWriteTime->get_TimeVal();

    if (::futimes(m_hFile, tv) == -1) return false;  // HResult::GetPOSIXLast(); ?

    return true;
#endif
}

bool cFile::SetFileTime(cTimeInt timeCreation, cTimeInt timeLastWrite) {
    //! Use HResult::GetLastDef() to find out why this fails.
    //! @return true = OK;
    if (!timeLastWrite.isTimeValid()) return false;

    cTimeFile LastWriteTime = timeLastWrite.GetAsFileTime();
    cTimeFile CreationTime = timeCreation.GetAsFileTime();
#ifdef _DEBUG
    if (timeLastWrite == timeCreation) {
        ASSERT(cMem::IsEqual(&CreationTime, &LastWriteTime, sizeof(LastWriteTime)));
    }
#endif
    return SetFileTime(&CreationTime, nullptr, &LastWriteTime);
}

HRESULT cFile::GetFileStatus(OUT cFileStatus& attr) const {
    //! Get the file status info by its open handle
    //! Similar to CFile::GetStatus()
    //! Same as cFileDir::ReadFileStatus()

    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);

#ifdef _WIN32
    ::BY_HANDLE_FILE_INFORMATION fi;
    if (!::GetFileInformationByHandle(get_Handle(), &fi)) {
        return HResult::GetLastDef(E_HANDLE);
    }

    attr.m_timeCreate = fi.ftCreationTime;                                         // m_ctime  = (may not be supported).
    attr.m_timeChange = fi.ftLastWriteTime;                                        // m_mtime = real world time/date of last modification. (accurate to 2 seconds)
    attr.m_timeLastAccess = fi.ftLastAccessTime;                                   // m_atime = time of last access/Open. (For Caching). (may not be supported)
    attr.m_Size = fi.nFileSizeLow | (CastN(FILE_SIZE_t, fi.nFileSizeHigh) << 32);  // file size in bytes.
    attr.m_Attributes = static_cast<FILEATTR_t>(fi.dwFileAttributes);              // Mask of ATTR_ attribute bits. ATTR_Normal

#elif defined(__linux__)

    cFileStatusSys statusSys;
    HRESULT hRes = GetStatusSys(statusSys);
    if (FAILED(hRes)) return hRes;

    attr.InitFileStatus(statusSys);
#endif

    return S_OK;
}

HRESULT cFile::ReadX(void* pData, size_t nDataSize) noexcept {  // virtual  
    //! Read a blob from the stream. advance the current position.
    //! @return
    //!  length of the read data. 0 or HRESULT_WIN32_C(ERROR_HANDLE_EOF) = end of file.

    if (nDataSize <= 0) return 0;
    if (pData == nullptr) {
        // just skip it.
        STREAM_POS_t nPosStart = GetPosition();
        HRESULT hRes = cOSHandle::SeekX(nDataSize, SEEK_t::_Cur);
        if (FAILED(hRes)) return hRes;

        STREAM_POS_t nPosCur = GetPosition();
        return CastN(HRESULT, nPosCur - nPosStart);  // <= nDataSize
    }

    return cOSHandle::ReadX(pData, nDataSize);
}

HRESULT cFile::WriteX(const void* pData, size_t nDataSize) {  // virtual 
    //! Write a blob to the stream. advance the current position.
    //! @arg
    //!  pData == nullptr = just test if i could write this much.
    //! @return
    //!  length written.
    //!  <0 = FAILED
    //!   ERROR_INVALID_USER_BUFFER = too many async calls ?? wait
    //!   ERROR_IO_PENDING = must wait!?

    if (nDataSize == 0) return 0;  // nothing to do.        
    if (pData == nullptr) {
        DEBUG_ASSERT(pData != nullptr, "Data null");
        return E_POINTER;
    }

    HRESULT hResLengthWritten = cOSHandle::WriteX(pData, nDataSize);
    if (FAILED(hResLengthWritten)) {
        if (hResLengthWritten == E_HANDLE) {  // handle is junk!! No idea why. Clear it.
            Close();
        }
        DEBUG_ASSERT(hResLengthWritten == CastN(HRESULT, nDataSize), "Write");
    }
    return CastN(HRESULT, hResLengthWritten);
}

HRESULT cFile::FlushX() {  // virtual
    //! synchronous flush of write data to file.
    if (!isValidHandle()) return S_OK;
    HRESULT hRes = cOSHandle::FlushX();
    if (FAILED(hRes)) {
        DEBUG_WARN(("File Flush failed"));
        // CFileException::ThrowOsError( HResult::GetLast(), m_strFileName);
        return hRes;
    }
    return S_OK;
}

HRESULT GRAYCALL cFile::DeletePath(const FILECHAR_t* pszFilePath) {  // static
    //! Use 'DeletePath' name because windows uses a "#define" macro to overload DeleteFile()!
    //! Same as MFC CFile::Remove()
    //! @note This can't be used with wildcards or to delete folders ! use cFileDir::DeleteDirFiles for that.
    //! @return
    //!  S_OK = 0.
    //!  S_FALSE = not here, but thats probably OK.
    //!  <0 = HRESULT failed for some other reason. (ERROR_ACCESS_DENIED,ERROR_PATH_NOT_FOUND)
    //!  Cant delete a directory this way !
#ifdef _WIN32
    if (!::DeleteFileW(cFilePath::GetFileNameLongW(pszFilePath)))
#else
    // same as ::unlink(). ENOENT or EACCES
    if (::remove(pszFilePath) != 0)
#endif
    {
        HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
        if (hRes == HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND)) return S_FALSE;
        return hRes;
    }
    return S_OK;
}

HRESULT GRAYCALL cFile::DeletePathX(const FILECHAR_t* pszPath, FILEOPF_t nFileFlags) {  // static
    //! Delete this file and fix collisions and read only marking.
    HRESULT hRes = cFile::DeletePath(pszPath);
#ifdef _WIN32
    if (hRes == E_ACCESSDENIED && (nFileFlags & FOF_RENAMEONCOLLISION)) {
        // remove read only flag. retry delete.
        // Try to change the attributes. then try delete again.
        hRes = cFileStatus::WriteFileAttributes(pszPath, FILEATTR_t::_Normal);
        if (SUCCEEDED(hRes)) {
            hRes = cFile::DeletePath(pszPath);
        }
    }
#endif
    return hRes;
}

HRESULT GRAYCALL cFile::LoadFile(const FILECHAR_t* pszFilePath, OUT cBlob& blob, size_t nSizeExtra) {  // static
    //! Read whole file into memory.
    //! @arg nSizeExtra = allocate some extra space at end.
    //! @return size read (Not including nSizeExtra). or <0 = error.
    cFile file;
    HRESULT hRes = file.OpenX(pszFilePath, OF_READ);
    if (FAILED(hRes)) return hRes;
    return file.ReadAll(blob, nSizeExtra);
}
}  // namespace Gray
