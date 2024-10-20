//! @file cFileDir.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cFileDir_H
#define _INC_cFileDir_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "HResult.h"
#include "cArrayString.h"
#include "cFile.h"
#include "cFileStatus.h"
#include "cString.h"
#include "cTimeInt.h"

#ifdef __linux__
#include <dirent.h>  // DIR
#endif

namespace Gray {
struct cLogProcessor;

#ifdef _WIN32
#define FILEDEVICE_PREFIX "\\\\.\\"  // usually _FN(FILEDEVICE_PREFIX). similar to "\\Device\\"
#elif defined(__linux__)
#define FILEDEVICE_PREFIX "/dev/"
#endif

/// <summary>
/// Info for a particular Disk/Device/Volume. e.g. 'C:\'
/// </summary>
struct GRAYCORE_LINK cFileDevice {
    static const char* const k_FileSysName[static_cast<int>(FILESYS_t::_QTY)];  /// File system types i might support.

    // _WIN32 Info from GetVolumeInformation();
    cStringF _sVolumeName;                      /// can be empty.
    cStringF _sTypeName;                        /// File system format/type e.g. "NTFS", "FAT"
    FILESYS_t _eSysType = FILESYS_t::_DEFAULT;  /// Enumerate known types for _sTypeName (file system type)
    UINT64 _nSerialNumber = 0;                  /// Volume serial number (time stamp of last format) e.g. 0x0ca0e613 for _WIN32.
    DWORD _nMaximumComponentLength = 0;         /// block size? e.g. 255 bytes
    bool _isCaseSensitive = false;              /// e.g. 0x03e700ff, FILE_CASE_SENSITIVE_SEARCH. else IgnoreCase

    /// <summary>
    /// Read info about device.
    //! pszDeviceId can be from _WIN32 GetLogicalDriveStrings()
    //! some drives won't be ready (if removable). Thats OK. HRESULT_WIN32_C(ERROR_NOT_READY)
    /// </summary>
    /// <param name="pszDeviceId">nullptr = use the current dir/path for the app.</param>
    HRESULT UpdateInfo(const FILECHAR_t* pszDeviceId = nullptr);

    FILESYS_t get_FileSysType() const noexcept {
        return _eSysType;
    }

    /// <summary>
    /// The file system is case sensitive ? __linux__ = true, _WIN32 = false
    /// A network mounted SAMBA share will use whatever rules the native OS/FileSystem uses.
    /// _sTypeName = "FAT","NTFS" system = non case sensitive. "NFS" = case sensitive.
    /// </summary>
    bool isCaseSensitive() const noexcept {
        return _isCaseSensitive;
    }

    /// <summary>
    /// Determines whether a disk drive is a removable, fixed, CD-ROM, RAM disk, or network drive.
    /// https://msdn.microsoft.com/en-us/library/windows/desktop/aa364939(v=vs.85).aspx
    /// </summary>
    /// <param name="pszDeviceId"></param>
    /// <returns>0 =DRIVE_UNKNOWN, 1=DRIVE_NO_ROOT_DIR, 2=DRIVE_REMOVABLE, 3=DRIVE_FIXED, 4=DRIVE_REMOTE, 5=DRIVE_CDROM, 6=DRIVE_RAMDISK</returns>
    static UINT GRAYCALL GetDeviceType(const FILECHAR_t* pszDeviceId);

    static FILE_SIZE_t GRAYCALL GetDeviceFreeSpace(const FILECHAR_t* pszDeviceId = nullptr);
    static HRESULT GRAYCALL GetSystemDeviceList(cArrayString<FILECHAR_t>& a);
};

/// <summary>
/// A file that is part of a directory listing.
/// </summary>
class GRAYCORE_LINK cFileDirEntry : public cFileStatus {
    typedef cFileStatus SUPER_t;
    typedef cFileDirEntry THIS_t;

    friend class cFileFind;

 protected:
    cStringF _sFileName;  /// relative file title. (NOT FULL PATH) if FILECHAR_t is NOT USE_UNICODE_FN then is UTF8.

 public:
    cFileDirEntry() noexcept {
        InitFileStatus();
    }
    explicit cFileDirEntry(const FILECHAR_t* pszFileName) : _sFileName(pszFileName) {
        InitFileStatus();
    }
    cFileDirEntry(const FILECHAR_t* pszFileName, const cFileStatus& status) : SUPER_t(status), _sFileName(pszFileName) {}

    inline cStringF get_Name() const noexcept {
        return _sFileName;
    }

    bool IsFileEqualTo(const THIS_t& rEntry) const noexcept {
        // Does file system use case ?
        if (!_sFileName.IsEqualNoCase(rEntry._sFileName)) return false;
        return SUPER_t::IsFileEqualTo(rEntry);
    }
    bool IsFileEqualTo(const THIS_t* pEntry) const noexcept {
        // Does file system use case ?
        if (pEntry == nullptr) return false;
        return IsFileEqualTo(*pEntry);
    }
    bool operator==(const THIS_t& rEntry) const noexcept {
        return IsFileEqualTo(rEntry);
    }
    bool operator!=(const THIS_t& rEntry) const noexcept {
        return !IsFileEqualTo(rEntry);
    }

    inline bool isDot() const {
        if (_sFileName[0] != '.') return false;
        if (_sFileName[1] == '\0') return true;
        return false;
    }
    inline bool isDots() const noexcept {
        //! ignore the . and .. that old systems can give us.
        if (_sFileName[0] != '.') return false;
        if (_sFileName[1] == '\0') return true;
        if (_sFileName[1] != '.') return false;
        if (_sFileName[2] == '\0') return true;
        return false;
    }
};

/// <summary>
/// Read/Browse/Enumerate directory in ongoing/serial state. use FindFileNext() to get next file.
/// Similar to MFC CFileFind.
/// @note Don't delete files while reading here. no idea what effect that has. Use cFileDir.
/// </summary>
class GRAYCORE_LINK cFileFind {
 public:
    cFileDirEntry _FileEntry;  /// The current entry. by calls to FindFile() and FindFileNext()

 private:
    cStringF _sDirPath;  /// Assume it ends with k_DirSep
    DWORD _nFileFlags = 0;   /// FILEOPF_t Options such as follow the links in the directory. Act as though these are regular files.
#ifdef _WIN32
    ::WIN32_FIND_DATAW _FindInfo;  /// Always UNICODE as base.
    ::HANDLE _hContext = INVALID_HANDLE_VALUE;  /// Handle for my search. NOT cOSHandle, uses FindClose()
#elif defined(__linux__)
    cStringF _sWildcardFilter;  /// Need to perform wildcard (strip out the *.EXT part) later/manually in Linux.
 public:
    bool _HasStats = true;  /// e.g. "/proc" directory has no extra stats. don't read them.
 private:
    ::DIR* _hContext = nullptr;  /// Handle for my search/enum.
#else
#error NOOS
#endif

 public:
    explicit cFileFind(cStringF sDirPath = _FN(""), DWORD nFileFlags = 0) noexcept;
    ~cFileFind() {
        CloseContext();
    }

    cStringF get_DirPath() const noexcept {
        return _sDirPath;
    }
    /// <summary>
    /// Create a full file path with directory and file name/title.
    /// </summary>
    cStringF GetFilePath(const FILECHAR_t* pszFileTitle) const {
        return cFilePath::CombineFilePathX(_sDirPath, pszFileTitle);
    }
    /// <summary>
    /// Get Full file path. like MFC CFileFind::GetFilePath()
    /// </summary>
    cStringF get_FilePath() const {
        return GetFilePath(_FileEntry.get_Name());
    }
    bool isDots() const noexcept {
        return _FileEntry.isDots();
    }

    /// <summary>
    /// start a sequential read of the files in a list of possible matches.
    /// @note pszWildcardFile can NOT have multiple "*.ext1;*.ext2"
    /// </summary>
    /// <param name="pszDirPath"></param>
    /// <param name="pszWildcardFile">"*.ext". if pszDirPath is empty, full path can be in pszWildcardFile</param>
    /// <returns>HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no files.</returns>
    HRESULT FindOpen(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr);

    HRESULT FindFile(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr);

    /// <summary>
    /// Read the next file in the directory list.
    /// ASSUME cFileFind::FindFile() was called.
    /// @note UNICODE files are converted to '?' chars if calling the non UNICODE version.
    /// </summary>
    /// <param name="bFirst"></param>
    /// <returns>HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no more files</returns>
    HRESULT FindFileNext(bool bFirst = false);

    bool isContextOpen() const;
    void CloseContext();
};

/// <summary>
/// A file folder or directory. read/cached as a single action.
/// Stores a list of the files as a single action.
/// @note i CAN delete files without harming the list. (unlike cFileFind)
/// </summary>
class GRAYCORE_LINK cFileDir {
 public:
    static const int k_FilesMax = 64 * 1024;
    static const LOGCHAR_t k_szCantMoveFile[];  /// if MoveDirFiles failed for this.

    cArrayStruct<cFileDirEntry> _aFiles;  /// Array of the files we found matching the ReadDir criteria.

 protected:
    cStringF _sDirPath;  /// Does NOT include the wild card.

 protected:
    /// <summary>
    /// add the file to a list. Overload this to do extra filtering.
    /// </summary>
    virtual HRESULT AddFileDirEntry(cFileDirEntry& fileEntry) {
        if (!fileEntry.isDots()) {
            _aFiles.Add(fileEntry);
        }
        return S_OK;
    }

 public:
    explicit cFileDir(cStringF sDirPath = _FN("")) : _sDirPath(sDirPath) {}
    virtual ~cFileDir() {}

    static HRESULT GRAYCALL RemoveDirectory1(const FILECHAR_t* pszDirName);
    static HRESULT GRAYCALL CreateDirectory1(const FILECHAR_t* pszDirName);
    static HRESULT GRAYCALL CreateDirectoryX(const FILECHAR_t* pszDirName, StrLen_t iStart = 0);
    static HRESULT GRAYCALL CreateDirForFileX(const FILECHAR_t* pszFilePath, StrLen_t iStart = 0);
    static HRESULT GRAYCALL MovePathToTrash(const FILECHAR_t* pszPath, bool bDir);

    static HRESULT GRAYCALL DirFileOps(FILEOP_t eOp, const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, FILEOPF_t nFileFlags, cLogProcessor* pLog, IStreamProgressCallback* pProgress);
    static HRESULT GRAYCALL MoveDirFiles(const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, cLogProcessor* pLog = nullptr, IStreamProgressCallback* pProgress = nullptr) {
        //! Move this directory and all its files.
        return DirFileOps(FILEOP_t::_Move, pszDirSrc, pszDirDest, CastN(FILEOPF_t, 0), pLog, pProgress);
    }
    static HRESULT GRAYCALL CopyDirFiles(const FILECHAR_t* pszDirSrc, const FILECHAR_t* pszDirDest, cLogProcessor* pLog = nullptr, IStreamProgressCallback* pProgress = nullptr) {
        //! Copy this directory and all its files.
        return DirFileOps(FILEOP_t::_Copy, pszDirSrc, pszDirDest, FILEOPF_t::_None, pLog, pProgress);
    }

    /// <summary>
    /// Delete this directory AND all its files.
    /// similar to cFileDirDlg::DeleteDirFiles( FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI )
    /// e.g. cFileDir::DeleteDirFiles( pszDirPath ); = delete directory and all its sub stuff.
    /// e.g. cFileDir::DeleteDirFiles( pszDirPath, "*.h" ); = delete contents of directory and all its wild carded children. leaves directory.
    /// HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND)
    /// </summary>
    static HRESULT GRAYCALL DeleteDirFiles(const FILECHAR_t* pszDirName, const FILECHAR_t* pszWildcardFile = nullptr, FILEOPF_t nFileFlags = FILEOPF_t::_None) {
        return DirFileOps(FILEOP_t::_Delete, pszDirName, pszWildcardFile, nFileFlags, nullptr, nullptr);
    }

    static HRESULT GRAYCALL DeletePathX(const FILECHAR_t* pszPath, FILEOPF_t nFileFlags = FILEOPF_t::_None);

    cStringF get_DirPath() const noexcept {
        return _sDirPath;
    }
    void put_DirPath(cStringF sDirPath) {
        _sDirPath = sDirPath;
        // clear list only if changed?
        RemoveAll();
    }

    /// <summary>
    /// rebuild Full path.
    /// </summary>
    cStringF GetFilePath(const FILECHAR_t* pszTitle) const {
        return cFilePath::CombineFilePathX(_sDirPath, pszTitle);
    }
    cStringF GetFilePath(const cFileDirEntry& f) const {
        return GetFilePath(f.get_Name());
    }

    const cFileDirEntry& GetEnumFile(ITERATE_t i) const noexcept {
        return _aFiles.GetAt(i);
    }
    /// <summary>
    /// Get the full path for the file i.
    /// </summary>
    cStringF GetEnumPath(ITERATE_t i) const {
        return GetFilePath(_aFiles.GetAt(i));
    }

    void RemoveAll() {
        //! Dispose of my data.
        _aFiles.RemoveAll();
    }

    HRESULT ReadDir(const FILECHAR_t* pszDirPath = nullptr, const FILECHAR_t* pszWildcardFile = nullptr, ITERATE_t iFilesMax = k_FilesMax, bool bFollowLink = false);

    HRESULT ReadDirAnyExt(const FILECHAR_t* pszFilePath, ITERATE_t iFilesMax = k_FilesMax);
    HRESULT ReadDirPreferredExt(const FILECHAR_t* pszFilePath, const cSpan<const FILECHAR_t*> exts);
};

#ifndef GRAY_STATICLIB  // force implementation/instantiate for DLL/SO.
template struct GRAYCORE_LINK cArrayStruct<cFileDirEntry>;
#endif
}  // namespace Gray
#endif  // _INC_cFileDir_H
