//! @file cFileCopier.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cFileCopier_H
#define _INC_cFileCopier_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cFile.h"
#include "cFileDir.h"

namespace Gray {
/// <summary>
/// An Abstract/interface to request/send a file from/to a file system (typically remote). (e.g. HTTP,FTP,another file device,etc)
/// And track it's progress.
/// similar to __linux__ curl lib
/// TODO Request to Stream ?? Send a Stream ?
/// </summary>
struct GRAYCORE_LINK DECLSPEC_NOVTABLE IFileCopier {
    IGNORE_WARN_ABSTRACT(IFileCopier);

    /// get a debug name for the connection/ file system.
    virtual cStringA get_ConnectName() const = 0;

    /// <summary>
    /// Connect to the device by its name. Maybe URI or File device to transfer files to/from. 
    /// ASSUME blocking call.
    /// </summary>
    /// <param name="pszDevice">can be prefixed by "http://host" or "ftp://host" or "C:\etc". nullptr = It might be needed to re-connect to a device if it disconnects spontaneously.</param>
    /// <returns>-gt- 0=success</returns>
    virtual HRESULT Connect(const FILECHAR_t* pszDevice) {
        UNREFERENCED_PARAMETER(pszDevice);
        return S_FALSE;  // not needed i guess.
    }

    /// <summary>
    /// Request a file from a (remote) server to be brought back to me/local.
    /// ASSUME blocking call.
    /// </summary>
    /// <param name="pszSrcRemote"></param>
    /// <param name="pszDestLocal">a local file. nullptr = query only. don't actually get the file.</param>
    /// <param name="pProgress"></param>
    /// <param name="nOffsetStart"></param>
    /// <param name="pnRequestSizeEst">nullptr = i don't care. -1 = request/return the size.</param>
    /// <returns></returns>
    virtual HRESULT RequestFile(const FILECHAR_t* pszSrcRemote, const FILECHAR_t* pszDestLocal, IStreamProgressCallback* pProgress = nullptr, FILE_SIZE_t nOffsetStart = 0, FILE_SIZE_t* pnRequestSizeEst = nullptr) {
        UNREFERENCED_PARAMETER(pszSrcRemote);
        UNREFERENCED_PARAMETER(pszDestLocal);
        UNREFERENCED_PARAMETER(pProgress);
        UNREFERENCED_PARAMETER(nOffsetStart);
        UNREFERENCED_PARAMETER(pnRequestSizeEst);
        return E_NOTIMPL;
    }

    /// <summary>
    /// Send a file to a remote/server from local storage. or delete the remote side file. (pszSrcPath=nullptr,nSize=0).
    /// ASSUME blocking call.
    /// @note I cannot set the modification time stamp for the file here.
    /// </summary>
    /// <param name="pszSrcLocal">a local file. nullptr = delete the DestName.</param>
    /// <param name="pszDestRemote">the remote side file path.</param>
    /// <param name="pProgress"></param>
    /// <param name="nOffsetStart"></param>
    /// <param name="nSize"></param>
    /// <returns></returns>
    virtual HRESULT SendFile(const FILECHAR_t* pszSrcLocal, const FILECHAR_t* pszDestRemote, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart = 0, FILE_SIZE_t nSize = (FILE_SIZE_t)-1) {
        UNREFERENCED_PARAMETER(pszSrcLocal);
        UNREFERENCED_PARAMETER(pszDestRemote);
        UNREFERENCED_PARAMETER(pProgress);
        UNREFERENCED_PARAMETER(nOffsetStart);
        UNREFERENCED_PARAMETER(nSize);
        return E_NOTIMPL;
    }

    /// <summary>
    /// Optionally set the remote side time stamp for a file. 
    /// ASSUME blocking call.
    /// </summary>
    /// <param name="pszDestName">the remote side file path.</param>
    /// <param name="timeChanged"></param>
    /// <returns></returns>
    virtual HRESULT SendAttr(const FILECHAR_t* pszDestName, cTimeFile timeChanged) {
        UNREFERENCED_PARAMETER(pszDestName);
        UNREFERENCED_PARAMETER(timeChanged);
        return E_NOTIMPL;
    }

    /// <summary>
    /// Optionally ask the remote for a directory listing.
    /// ASSUME blocking call.
    /// </summary>
    /// <param name="pszDir"></param>
    /// <param name=""></param>
    /// <returns>-lt- 0 = error</returns>
    virtual HRESULT RequestDirectory(const FILECHAR_t* pszDir, OUT cArrayStruct<cFileDirEntry>& dirRet) {
        UNREFERENCED_PARAMETER(pszDir);
        UNREFERENCED_PARAMETER(dirRet);
        return E_NOTIMPL;
    }
};

/// <summary>
/// Implement the IFileCopier for the local file system. Copy single files.
/// </summary>
class GRAYCORE_LINK cFileCopier : public IFileCopier {
 public:
    cStringF _sRemoteRoot;  /// Prefix all server/remote side (non local) paths with this.

 protected:
    cStringF MakeRemotePath(const FILECHAR_t* pszFileName) const {
        return cFilePath::CombineFilePathX(_sRemoteRoot, pszFileName);
    }

 public:
    /// <summary>
    /// get a debug name for the server / non local connection.
    /// </summary>
    /// <returns></returns>
    cStringA get_ConnectName() const override {
        return "File";
    }

    /// <summary>
    /// Connect to the "remote" side 
    /// </summary>
    /// <param name="pszRemoteRoot">server/Remote side names can be prefixed by "C:\etc"</param>
    /// <returns></returns>
    HRESULT Connect(const FILECHAR_t* pszRemoteRoot) override {
        _sRemoteRoot = pszRemoteRoot;
        return S_OK;
    }

    HRESULT RequestFile(const FILECHAR_t* pszSrcRemote, const FILECHAR_t* pszDestLocal, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t* pnRequestSizeEst) override;
    HRESULT SendFile(const FILECHAR_t* pszSrcLocal, const FILECHAR_t* pszDestRemote, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t nSize) override;
    HRESULT SendAttr(const FILECHAR_t* pszDestName, cTimeFile timeChanged) override;
    HRESULT RequestDirectory(const FILECHAR_t* pszDir, OUT cArrayStruct<cFileDirEntry>& dirRet) override;

#if defined(_WIN32) && !defined(UNDER_CE)
    static DWORD CALLBACK onCopyProgressCallback(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber,
                                                 DWORD dwCallbackReason,  // CALLBACK_CHUNK_FINISHED or CALLBACK_STREAM_SWITCH
                                                 ::HANDLE hSourceFile, ::HANDLE hDestinationFile, void* lpData);
#endif

    static HRESULT GRAYCALL CopyFileStream(cStreamInput& stmIn, const FILECHAR_t* pszDstFileName, bool bFailIfExists = false, IStreamProgressCallback* pProgress = nullptr, FILE_SIZE_t nOffsetStart = 0);

    static HRESULT GRAYCALL CopyFileX(const FILECHAR_t* pszSrcName, const FILECHAR_t* pszDstName, IStreamProgressCallback* pProgress = nullptr, bool bFailIfExists = false);
    static HRESULT GRAYCALL RenamePath(const FILECHAR_t* pszOldName, const FILECHAR_t* pszNewName, IStreamProgressCallback* pProgress = nullptr);
};
}  // namespace Gray
#endif
