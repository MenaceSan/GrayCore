//
//! @file cFileText.cpp
//! see http://www.codeproject.com/file/handles.asp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cFileText.h"
#include "cLogMgr.h"

#if USE_CRT
#if defined(_WIN32) && !defined(UNDER_CE)
#include <io.h>
#endif
#if !defined(UNDER_CE)
#include <fcntl.h>  // O_TEXT
#endif
#endif

namespace Gray {
HRESULT cFileTextBase::ReadStringLineA(OUT cStringA& r) {
    char szTmp[StrT::k_LEN_Default];
    HRESULT hRes = ReadStringLine(szTmp, STRMAX(szTmp));
    if (FAILED(hRes)) return hRes;
    r = szTmp;
    return hRes;
}

#if USE_CRT
#if defined(UNDER_CE) || defined(__linux__)
typedef HANDLE FILEDESC_t;  /// Posix file descriptor id. return value for _fileno() is same as HANDLE in __linux__ and UNDER_CE
#else
typedef int FILEDESC_t;     /// Posix file descriptor id for std C. used for separate _fileno in FILE*. return value for fileno()
#endif

cFileText::cFileText() {}
cFileText::cFileText(cStringF sFilePath, OF_FLAGS_t nOpenFlags) {
    OpenX(sFilePath, nOpenFlags);
}
cFileText::~cFileText() noexcept {
    // Virtuals don't work in destructors !
    Close();
}

const FILECHAR_t* cFileText::get_ModeCPtr() const {
    //! get the proper fopen() mode arguments.
    const UINT nOpenFlags = get_ModeFlags();

    if (nOpenFlags & OF_READWRITE)  // "r+b"
        return _FN("ab+");

    if (nOpenFlags & OF_CACHE_SEQ) {
        // Only used sequentially.
        if (isModeWrite()) {
            return _FN("wbS");
        }
        return _FN("rbS");
    }

    if (!(nOpenFlags & OF_TEXT))  // OF_BINARY
    {
        return isModeWrite() ? _FN("wb") : _FN("rb");
    }

    // text modes. (Never do "\r\n" translation?! it messes up the ftell()
    if (nOpenFlags & OF_CREATE) return _FN("w");
    if (isModeWrite())
        return _FN("w");  // "wb+"
    else
        return _FN("r");
}

HRESULT cFileText::OpenFileHandle(HANDLE h, OF_FLAGS_t nOpenFlags) {
    // Use _fdopen() to get the backing OSHandle.
    m_nOpenFlags = nOpenFlags;

#ifdef __GNUC__
    m_pStream = ::fdopen((FILEDESC_t)h, StrArg<char>(get_ModeCPtr()));
#else
    FILEDESC_t hConHandle = ::_open_osfhandle((intptr_t)h, O_TEXT);  // convert OS HANDLE to C file int handle. _O_TEXT
    m_pStream = ::_fdopen(hConHandle, StrArg<char>(get_ModeCPtr()));
#endif
    if (m_pStream == nullptr) {
        return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);  // Why ??
    }
    int iRet = ::setvbuf(m_pStream, nullptr, _IONBF, 0);  //
    if (iRet != 0) {
        // invalid mode parameter or to some other error allocating or assigning the buffer.
        return HResult::GetPOSIXLastDef(E_FAIL);
    }
    sm_iFilesOpen++;
    AttachHandle(h);
    return S_OK;
}

HRESULT cFileText::OpenX(cStringF sFilePath, OF_FLAGS_t nOpenFlags) {
    m_iCurLineNum = 0;

    HRESULT hRes = OpenSetup(sFilePath, nOpenFlags);
    if (FAILED(hRes)) {
        return hRes;
    }

    ASSERT(m_pStream == nullptr);
    const FILECHAR_t* pszMode = get_ModeCPtr();

    // _wfsopen() for wchar_t
#if !defined(UNDER_CE) && (_MSC_VER >= 1400)
#if USE_UNICODE_FN
    errno_t eRet = ::_wfopen_s(&m_pStream, sFilePath, pszMode);
#else
    errno_t eRet = ::fopen_s(&m_pStream, sFilePath, pszMode);
#endif
#else
#if USE_UNICODE_FN
    m_pStream = ::_wfopen(sFilePath, pszMode);
#else
    m_pStream = ::fopen(sFilePath, pszMode);
#endif
#endif
    if (m_pStream == nullptr) {
#if !defined(UNDER_CE) && (_MSC_VER >= 1400)
        return HResult::FromPOSIX(eRet);
#else
        // Assume HResult::GetLast() was set.
        return HResult::GetLastDef();
#endif
    }

    sm_iFilesOpen++;
    DEBUG_CHECK(sm_iFilesOpen >= 0);

    // Get the low level handle for it. m_hFile cFile functions use it instead.
    // NOTE: the POSIX 'int' handle (low level file descriptor) is not the same as the OS HANDLE in windows.
#if defined(_MSC_VER)
    FILEDESC_t iFileNo = ::_fileno(m_pStream);
#else
    FILEDESC_t iFileNo = fileno(m_pStream);           // macro
#endif

#if defined(__linux__) || defined(UNDER_CE)
    AttachHandle(iFileNo);
#else
    AttachHandle((HANDLE)::_get_osfhandle(iFileNo));  // _fileno is macro.
#endif
    return S_OK;
}

void cFileText::Close() noexcept {  // virtual
    // virtuals don't work in destruct.
    if (!isValidHandle()) return;

    if (isModeWrite()) {
        ::fflush(m_pStream);
    }
    bool bSuccess = (::fclose(m_pStream) == 0);
    DEBUG_CHECK(bSuccess);
    UNREFERENCED_PARAMETER(bSuccess);
    m_pStream = nullptr;

    DetachHandle();
}

HRESULT cFileText::SeekX(STREAM_OFFSET_t offset, SEEK_ORIGIN_TYPE eSeekOrigin) noexcept {  // virtual
    //! eSeekOrigin = SEEK_SET, SEEK_END
    //! @note end of line translation might be broken? ftell() and fseek() don't work correctly when you use it.
    //! @note offset < 0 for SEEK_Cur is legal.
    //! @return
    //!  <0 = FAILED
    //!  new file pointer position % int32.
    if (!isValidHandle()) return E_HANDLE;
    if (::fseek(m_pStream, (long)offset, eSeekOrigin) != 0) return HResult::GetLastDef();

    if (eSeekOrigin == SEEK_Set)  // SEEK_SET = FILE_BEGIN
    {
        if (offset == 0) {
            m_iCurLineNum = 0;  // i actually know the line number for this position.
        }
        return (HRESULT)offset;
    }
    m_iCurLineNum = k_ITERATE_BAD;  // invalid. no idea what the line number is now!
    return S_OK;
}

FILE* cFileText::DetachFileStream() noexcept {
    FILE* pStream = m_pStream;
    DetachHandle();
    m_pStream = nullptr;
    return pStream;
}

STREAM_POS_t cFileText::GetPosition() const {  // virtual
    //! override cStream
    //! @note end of line translation might be broken? ftell and fseek don't work correctly when you use it.
    //! @return -1 = error.
    if (!isValidHandle()) return k_STREAM_POS_ERR;
    return ::ftell(m_pStream);
}

HRESULT cFileText::FlushX() {  // virtual
    if (!isValidHandle()) return S_OK;
    ASSERT(m_pStream != nullptr);
    int iRet = ::fflush(m_pStream);
    if (iRet != 0)  // EOF
    {
        return HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
    }
    return S_OK;
}

bool cFileText::isEOF() const {
    if (!isValidHandle()) return true;
    return ::feof(m_pStream) ? true : false;
}

HRESULT cFileText::GetStreamError() const {
    // errno in M$
    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
    return HResult::FromPOSIX(::ferror(m_pStream));
}

HRESULT cFileText::ReadX(void* pBuffer, size_t nSizeMax) noexcept {  // virtual
    //! cStream
    //! Read a block of binary data or as much as we can until end of file.
    //! @return
    //!  the number of bytes actually read.
    //!  <0 = failed. HRESULT_WIN32_C(ERROR_READ_FAULT) HRESULT_WIN32_C(ERROR_HANDLE_EOF)
    if (pBuffer == nullptr) return E_INVALIDARG;
    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);

    if (isEOF()) return 0;  // LINUX will ASSERT if we read past end.
    size_t uRet = ::fread(pBuffer, 1, nSizeMax, m_pStream);
    ASSERT(uRet >= 0);
    if (uRet > nSizeMax) {
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_READ_FAULT));
    }
    // m_iCurLineNum++; // no idea.
    return CastN(HRESULT,uRet);  // size we got. 0 = end of file?
}

HRESULT cFileText::WriteX(const void* pData, size_t nDataSize) {  // virtual
    //! cStream
    //! Write binary data to the file.
    //! all or fail.
    //! @return >0 = success else fail.
    if (pData == nullptr) return E_INVALIDARG;

    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
    size_t uRet = ::fwrite(pData, nDataSize, 1, m_pStream);
    if (uRet != 1) {
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_WRITE_FAULT));
    }

    m_iCurLineNum++;  // count lines ??

    return CastN(HRESULT, nDataSize);
}

HRESULT cFileText::WriteString(const char* pszStr) {  // virtual
    //! override cStream
    //! @note If we did fgets() it will translate the \n for us.
    //!  so we must do the same on the output side.
    //! like .NET StreamWriter.WriteLine()
    //! @return <0 = failed.
    //!  Length of write in chars

    if (pszStr == nullptr) return E_INVALIDARG;
    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);

    if (::fputs(pszStr, m_pStream) < 0) {
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_WRITE_FAULT));
    }

    m_iCurLineNum++;  // count lines ??
    return 1;
}

#if 0
	HRESULT cFileText::ReadStringLine(wchar_t* pszBuffer, StrLen_t iSizeMax) { // virtual
		//! override cStream
		//! Read a single line of text. like fgets()
		//! @return length of the string read in chars. (includes \r\n) (not including '\0')
		ASSERT(0);
		return E_NOTIMPL;
	}
#endif

HRESULT cFileText::ReadStringLine(char* pszBuffer, StrLen_t iSizeMax) {  // virtual
    //! override cStream
    //! Read a line of text. like fgets().
    //! Read up until (including) newline character = \n = The newline character, if read, is included in the string.
    //! like .NET StreamReader.ReadLine
    //! @return length of the string read in chars. (includes \r\n) (not including null)
    //!  0 = EOF (legit end of file).
    //!  < 0 = other error

    if (pszBuffer == nullptr) return E_INVALIDARG;

    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
    if (isEOF()) {
        return 0;  // __linux__ will ASSERT if we read past end.
    }
    char* pszRet = ::fgets(pszBuffer, iSizeMax, m_pStream);
    if (pszRet == nullptr) {
        if (isEOF()) {  // legit end of file
            return 0;
        }
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_READ_FAULT));
    }

    if (m_iCurLineNum >= 0) m_iCurLineNum++;
    return StrT::Len(pszBuffer);  // Return length in chars.
}

bool cFileText::put_TextPos(const cTextPos& rPos) {
    // FILE_BEGIN == SEEK_SET
    if (!rPos.isValidPos()) return false;
    HRESULT hRes = SeekX(rPos.get_Offset(), SEEK_Set);
    if (FAILED(hRes)) return false;
    m_iCurLineNum = rPos.get_LineNum();
    return true;
}
#endif
}  // namespace Gray
