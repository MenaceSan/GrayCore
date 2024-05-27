//! @file cFileCopier.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cFileCopier.h"
#include "cFileDir.h"

namespace Gray {
HRESULT GRAYCALL cFileCopier::CopyFileStream(cStreamInput& stmIn, const FILECHAR_t* pszDstFileName, bool bFailIfExists, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart) {
    //! Copy this (opened OF_READ) file to some other file name/path. (pszDstFileName)
    //! manually read/copy the contents of the file via WriteStream().
    //! Similar effect to cFile::CopyFileX()

    HRESULT hRes;
    cFile fileDst;
    if (bFailIfExists) {
        // Check if it exists first.
        hRes = fileDst.OpenX(pszDstFileName, OF_READ | OF_BINARY | OF_EXIST);
        if (FAILED(hRes)) return hRes;  // can't overwrite!
        fileDst.Close();
    }

    hRes = fileDst.OpenX(pszDstFileName, OF_WRITE | OF_BINARY | OF_CREATE);
    if (FAILED(hRes)) return hRes;

    // ELSE ASSUME stmIn.SeekToBegin(); if appropriate.
    if (nOffsetStart > 0) {
        hRes = stmIn.SeekX(nOffsetStart, SEEK_t::_Set);
        if (FAILED(hRes)) return hRes;
        hRes = fileDst.SeekX(nOffsetStart, SEEK_t::_Set);
        if (FAILED(hRes)) return hRes;
    }

    hRes = fileDst.WriteStream(stmIn, fileDst.GetLength(), pProgress);
    if (FAILED(hRes)) return hRes;

    return S_OK;
}

#if defined(_WIN32) && !defined(UNDER_CE)
DWORD CALLBACK cFileCopier::onCopyProgressCallback(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber,
                                                   DWORD dwCallbackReason,  // CALLBACK_CHUNK_FINISHED or CALLBACK_STREAM_SWITCH
                                                   HANDLE hSourceFile, HANDLE hDestinationFile,
                                                   void* lpData) {  // static
    //! CopyProgressRoutine Callback Function forwarded to IStreamProgressCallback
    //! http://msdn.microsoft.com/en-us/library/aa363854(VS.85).aspx
    //! LPPROGRESS_ROUTINE called by CopyFileStream() in the CopyFileEx() case
    //! @return PROGRESS_STOP
    UNREFERENCED_PARAMETER(hDestinationFile);
    UNREFERENCED_PARAMETER(hSourceFile);
    UNREFERENCED_PARAMETER(dwCallbackReason);
    UNREFERENCED_PARAMETER(dwStreamNumber);
    UNREFERENCED_PARAMETER(StreamBytesTransferred);
    UNREFERENCED_PARAMETER(StreamSize);
    if (lpData != nullptr) {
        ASSERT(TotalBytesTransferred.QuadPart <= 0xffffffff);
        ASSERT(TotalFileSize.QuadPart <= 0xffffffff);  // truncation!
        HRESULT hRes = PtrCast<IStreamProgressCallback>(lpData)->onProgressCallback(cStreamProgress((STREAM_POS_t)TotalBytesTransferred.QuadPart, (STREAM_POS_t)TotalFileSize.QuadPart));
        if (FAILED(hRes)) {
            return PROGRESS_STOP;
        }
    }
    return PROGRESS_CONTINUE;
}
#endif

HRESULT GRAYCALL cFileCopier::CopyFileX(const FILECHAR_t* pszSrcName, const FILECHAR_t* pszDstName, IStreamProgressCallback* pProgress, bool bFailIfExists) {  // static
    //! OS Copy a file from pszSrcName to pszDstName. The pszDstName may or may not already exist. In the old days Windows used LZCopy for this.
    //! @note you may want to call WriteFileTimes() after this.
    //! Does NOT create missing child directories.
    //! @return
    //!  ERROR_REQUEST_ABORTED = canceled by callback.

#if defined(_WIN32) && !defined(UNDER_CE)
    bool bRet;
    if (pProgress != nullptr) {
        BOOL fCancel = false;
        bRet = ::CopyFileExW(cFilePath::GetFileNameLongW(pszSrcName),  // fix long name problems.
                             cFilePath::GetFileNameLongW(pszDstName), cFileCopier::onCopyProgressCallback, pProgress, &fCancel, bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0);
    } else {
        bRet = ::CopyFileW(cFilePath::GetFileNameLongW(pszSrcName), cFilePath::GetFileNameLongW(pszDstName), bFailIfExists);
    }
    if (!bRet) {
        const HRESULT hRes = HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
        if (hRes == HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND) && !bFailIfExists) {
            // Create the dest folder?
        }
        return hRes;
    }
    return S_OK;
#else
    // copyfile() ?? in C++17
    // sendfile()  will send up to 2G

    // Fallback to stream file.
    cFile fileSrc;
    HRESULT hRes = fileSrc.OpenX(pszSrcName, OF_READ | OF_BINARY);
    if (FAILED(hRes)) return hRes;
    return CopyFileStream(fileSrc, pszDstName, bFailIfExists, pProgress, 0);
#endif
}

HRESULT GRAYCALL cFileCopier::RenamePath(const FILECHAR_t* lpszOldName, const FILECHAR_t* lpszNewName, IStreamProgressCallback* pProgress) {  // static
    //! Equivalent of moving a file. (or directory and its children)
    //! A new directory must be on the same drive.
    //! @note Can't move file from once device to another! without MOVEFILE_COPY_ALLOWED
    //! @return
    //!  S_OK = 0 = good.
    //!  <0 = failed.

    bool bRet;
#if defined(__linux__)
    bRet = (::rename(lpszOldName, lpszNewName) == 0);  // POSIX
#elif defined(UNDER_CE) || defined(__GNUC__)
    bRet = ::MoveFileW(StrArg<wchar_t>(lpszOldName), StrArg<wchar_t>(lpszNewName));
#else
    bRet = ::MoveFileWithProgressW(cFilePath::GetFileNameLongW(lpszOldName), cFilePath::GetFileNameLongW(lpszNewName), cFileCopier::onCopyProgressCallback, pProgress, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
#endif
    if (!bRet) {
        // Will use POSIX error if __linux__
        return HResult::GetLastDef(HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND));
    }
    return S_OK;
}

HRESULT cFileCopier::RequestFile(const FILECHAR_t* pszSrcRemote, const FILECHAR_t* pszDestLocal, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t* pnRequestSizeEst) {  // virtual
    //! Request a file from a m_sRemoteRoot/pszSrcName (file system) to be brought back to me at local pszDestPath.
    //! @arg pnRequestSizeEst = unused/unnecessary for local file system copy.

    cStringF sSrcRemote = MakeRemotePath(pszSrcRemote);
    const bool bRequestSize = (pnRequestSizeEst != nullptr && *pnRequestSizeEst == (FILE_SIZE_t)-1);
    const bool bDestEmpty = StrT::IsWhitespace(pszDestLocal);
    if (bDestEmpty || bRequestSize) {
        // just retrieve the size of the file in pnRequestSizeEst using cFileStatus
        cFileStatus fs;
        const HRESULT hRes = fs.ReadFileStatus(sSrcRemote);
        if (FAILED(hRes)) return hRes;
        if (pnRequestSizeEst == nullptr) return E_POINTER;
        // return its size.
        *pnRequestSizeEst = fs.GetFileLength();
        if (bDestEmpty) return S_OK;
    }
    if (nOffsetStart != 0) {
        // A partial copy of the file.
        cFile fileSrc;
        const HRESULT hRes = fileSrc.OpenX(sSrcRemote, OF_READ | OF_BINARY);
        if (FAILED(hRes)) return hRes;
        return cFileCopier::CopyFileStream(fileSrc, pszDestLocal, false, pProgress, nOffsetStart);
    }
    return cFileCopier::CopyFileX(sSrcRemote, pszDestLocal, pProgress, false);
}

HRESULT cFileCopier::SendFile(const FILECHAR_t* pszSrcLocal, const FILECHAR_t* pszDestRemote, IStreamProgressCallback* pProgress, FILE_SIZE_t nOffsetStart, FILE_SIZE_t nSize) {  // override virtual
    //! Send a local file to a m_sRemoteRoot/pszDestName from local pszSrcPath storage
    //! @note I cannot set the modification time stamp for the file here.

    UNREFERENCED_PARAMETER(nSize);
    if (StrT::IsWhitespace(pszDestRemote)) return E_INVALIDARG;

    cStringF sDestRemote = MakeRemotePath(pszDestRemote);
    if (StrT::IsWhitespace(pszSrcLocal)) {
        // Acts like a delete. delete file or directory recursively.
        return cFileDir::DeletePathX(sDestRemote, FILEOPF_t::_None);
    }
    if (nOffsetStart != 0) {
        // A partial copy of the file.
        cFile fileSrc;
        const HRESULT hRes = fileSrc.OpenX(pszSrcLocal, OF_READ | OF_BINARY);
        if (FAILED(hRes)) return hRes;
        return cFileCopier::CopyFileStream(fileSrc, sDestRemote, false, pProgress, nOffsetStart);
    }

    // Whole file.
    HRESULT hRes = cFileCopier::CopyFileX(pszSrcLocal, sDestRemote, pProgress, false);
    if (hRes == HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND)) {
        // Need to create the path first
        hRes = cFileDir::CreateDirForFileX(sDestRemote, m_sRemoteRoot.GetLength());
        if (FAILED(hRes)) return hRes;
        hRes = cFileCopier::CopyFileX(pszSrcLocal, sDestRemote, pProgress, false);  // try again.
    }

    return hRes;
}

HRESULT cFileCopier::SendAttr(const FILECHAR_t* pszDestName, cTimeFile timeChanged) {  // override virtual
    //! Optionally set the remote side time stamp for a file.
    return cFileStatus::WriteFileTimes(MakeRemotePath(pszDestName), &timeChanged, &timeChanged);
}

HRESULT cFileCopier::RequestDirectory(const FILECHAR_t* pszDir, OUT cArrayStruct<cFileDirEntry>& dirRet) {  // override;
    cFileDir dir(pszDir);
    HRESULT hRes = dir.ReadDir();
    if (FAILED(hRes)) return hRes;
    dirRet = dir.m_aFiles;
    return hRes;
}
}  // namespace Gray
