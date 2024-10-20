//! @file cFileText.cpp
//! see http://www.codeproject.com/file/handles.asp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
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

#if USE_CRT
template <>
GRAYCORE_LINK void CloseHandleType(::FILE* p) noexcept {
    ::fclose(p);  // ignored BOOL return.
}
#endif

HRESULT cFileTextBase::ReadStringLineA(OUT cStringA& r) {
    char szTmp[StrT::k_LEN_Default];
    HRESULT hRes = ReadStringLine(TOSPAN(szTmp));
    if (FAILED(hRes)) return hRes;
    r = szTmp;
    return hRes;
}

#if USE_CRT
#if defined(UNDER_CE) || defined(__linux__)
typedef HANDLE FILEDESC_t;  /// Posix file descriptor id. return value for _fileno() is same as HANDLE in __linux__ and UNDER_CE
#else
typedef int FILEDESC_t;                                              /// Posix file descriptor id for std C. used for separate _fileno in FILE*. return value for fileno()
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
    if (nOpenFlags & OF_READWRITE) return _FN("ab+");  // "r+b"

    if (nOpenFlags & OF_CACHE_SEQ) {
        // Only used sequentially.
        if (isModeWrite()) return _FN("wbS");
        return _FN("rbS");
    }

    if (!(nOpenFlags & OF_TEXT))  return isModeWrite() ? _FN("wb") : _FN("rb"); // OF_BINARY

    // text modes. (Never do "\r\n" translation?! it messes up the ftell()
    if (nOpenFlags & OF_CREATE) return _FN("w");
    if (isModeWrite()) return _FN("w");  // "wb+"
    return _FN("r");
}

HRESULT cFileText::OpenFileHandle(HANDLE h, OF_FLAGS_t nOpenFlags) {
    // Use _fdopen() to get the backing OSHandle.
    _nOpenFlags = nOpenFlags;

#ifdef __GNUC__
    _pStream = ::fdopen((FILEDESC_t)h, StrArg<char>(get_ModeCPtr()));
#else
    const FILEDESC_t hConHandle = ::_open_osfhandle((intptr_t)h, O_TEXT);  // convert OS HANDLE to C file int handle. _O_TEXT
    _pStream = ::_fdopen(hConHandle, StrArg<char>(get_ModeCPtr()));
#endif
    if (_pStream == nullptr)  return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);  // Why ??

    const int iRet = ::setvbuf(_pStream, nullptr, _IONBF, 0);
    // invalid mode parameter or to some other error allocating or assigning the buffer.
    if (iRet != 0) return HResult::GetPOSIXLastDef(E_FAIL);

    sm_iFilesOpen++;
    AttachHandle(h);
    return S_OK;
}

HRESULT cFileText::OpenX(cStringF sFilePath, OF_FLAGS_t nOpenFlags) {
    _iLineNumCur = 0;

    HRESULT hRes = OpenSetup(sFilePath, nOpenFlags);
    if (FAILED(hRes)) return hRes;

    ASSERT(_pStream == nullptr);
    const FILECHAR_t* pszMode = get_ModeCPtr();

    // _wfsopen() for wchar_t
#if !defined(UNDER_CE) && (_MSC_VER >= 1400)
#if USE_UNICODE_FN
    const errno_t eRet = ::_wfopen_s(&_pStream, sFilePath, pszMode);
#else
    const errno_t eRet = ::fopen_s(&_pStream, sFilePath, pszMode);
#endif
#else
#if USE_UNICODE_FN
    _pStream = ::_wfopen(sFilePath, pszMode);
#else
    _pStream = ::fopen(sFilePath, pszMode);
#endif
#endif
    if (_pStream == nullptr) {
#if !defined(UNDER_CE) && (_MSC_VER >= 1400)
        return HResult::FromPOSIX(eRet);
#else
        // Assume HResult::GetLast() was set.
        return HResult::GetLastDef();
#endif
    }

    sm_iFilesOpen++;
    DEBUG_CHECK(sm_iFilesOpen >= 0);

    // Get the low level handle for it. _hFile cFile functions use it instead.
    // NOTE: the POSIX 'int' handle (low level file descriptor) is not the same as the OS HANDLE in windows.
#if defined(_MSC_VER)
    const FILEDESC_t iFileNo = ::_fileno(_pStream);
#else
    const FILEDESC_t iFileNo = fileno(_pStream);  // macro
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
        ::fflush(_pStream);
    }
    const bool bSuccess = (::fclose(_pStream) == 0);
    DEBUG_CHECK(bSuccess);
    UNREFERENCED_PARAMETER(bSuccess);
    _pStream = nullptr;

    DetachHandle();
}

HRESULT cFileText::SeekX(STREAM_OFFSET_t offset, SEEK_t eSeekOrigin) noexcept {  // virtual
    //! eSeekOrigin = SEEK_SET, SEEK_END
    //! @note end of line translation might be broken? ftell() and fseek() don't work correctly when you use it.
    //! @note offset < 0 for SEEK_t::_Cur is legal.
    //! @return
    //!  <0 = FAILED
    //!  new file pointer position % int32.
    if (!isValidHandle()) return E_HANDLE;
    if (::fseek(_pStream, (long)offset, (int)eSeekOrigin) != 0) return HResult::GetLastDef();

    if (eSeekOrigin == SEEK_t::_Set) {  // SEEK_SET = FILE_BEGIN
        if (offset == 0) {
            _iLineNumCur = 0;  // i actually know the line number for this position.
        }
        return CastN(HRESULT, offset);
    }
    _iLineNumCur = k_ITERATE_BAD;  // invalid. no idea what the line number is now!
    return S_OK;
}

FILE* cFileText::DetachFileStream() noexcept {
    FILE* pStream = _pStream;
    DetachHandle();
    _pStream = nullptr;
    return pStream;
}

STREAM_POS_t cFileText::GetPosition() const noexcept {  // virtual
    //! override cStream
    //! @note end of line translation might be broken? ftell and fseek don't work correctly when you use it.
    //! @return -1 = error.
    if (!isValidHandle()) return k_STREAM_POS_ERR;
    return ::ftell(_pStream);
}

HRESULT cFileText::FlushX() {  // virtual
    if (!isValidHandle()) return S_OK;
    ASSERT(_pStream != nullptr);
    const int iRet = ::fflush(_pStream);
    if (iRet != 0) {  // EOF
        return HResult::GetPOSIXLastDef(HRESULT_WIN32_C(ERROR_WRITE_FAULT));
    }
    return S_OK;
}

bool cFileText::isEOF() const {
    if (!isValidHandle()) return true;
    return ::feof(_pStream) ? true : false;
}

HRESULT cFileText::GetStreamError() const {
    // errno in M$
    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
    return HResult::FromPOSIX(::ferror(_pStream));
}

HRESULT cFileText::ReadX(cMemSpan ret) noexcept {  // virtual
    //! cStream
    //! Read a block of binary data or as much as we can until end of file.
    //! @return
    //!  the number of bytes actually read.
    //!  <0 = failed. HRESULT_WIN32_C(ERROR_READ_FAULT) HRESULT_WIN32_C(ERROR_HANDLE_EOF)

    if (ret.isNull()) return E_INVALIDARG;
    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
    if (isEOF()) return 0;  // LINUX will ASSERT if we read past end.

    const size_t uRet = ::fread(ret.get_BytePtrW(), 1, ret.get_SizeBytes(), _pStream);
    ASSERT(uRet >= 0);
    if (uRet > ret.get_SizeBytes()) {
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_READ_FAULT));
    }
    // _iLineNumCur++; // no idea how many lines.
    return CastN(HRESULT, uRet);  // size we got. 0 = end of file?
}

HRESULT cFileText::WriteX(const cMemSpan& m) {  // virtual
    //! cStream
    //! Write binary data to the file.
    //! all or fail.
    //! @return >0 = success else fail.
    if (m.isNull()) return E_INVALIDARG;

    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
    size_t uRet = ::fwrite(m, m.get_SizeBytes(), 1, _pStream);
    if (uRet != 1) {
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_WRITE_FAULT));
    }

    _iLineNumCur++;  // count lines ?? ASSUME it was just 1 line ???
    return CastN(HRESULT, m.get_SizeBytes());
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

    if (::fputs(pszStr, _pStream) < 0) {
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_WRITE_FAULT));
    }

    _iLineNumCur++;  // count lines ??
    return 1;
}

#if 0
	HRESULT cFileText::ReadStringLine(cSpanX<wchar_t> ret ) { // virtual
		//! override cStream
		//! Read a single line of text. like fgets()
		//! @return length of the string read in chars. (includes \r\n) (not including '\0')
		ASSERT(0);
		return E_NOTIMPL;
	}
#endif

HRESULT cFileText::ReadStringLine(cSpanX<char> ret) {  // virtual
    //! override cStream
    //! Read a line of text. like fgets().
    //! Read up until (including) newline character = \n = The newline character, if read, is included in the string.
    //! like .NET StreamReader.ReadLine
    //! @return length of the string read in chars. (includes \r\n) (not including null)
    //!  0 = EOF (legit end of file).
    //!  < 0 = other error

    if (ret.isNull()) return E_INVALIDARG;
    if (!isValidHandle()) return HRESULT_WIN32_C(ERROR_INVALID_TARGET_HANDLE);
    if (isEOF()) return 0;  // __linux__ will ASSERT if we read past end.

    char* pszRet = ::fgets(ret.get_PtrWork(), ret.get_MaxLen(), _pStream);
    if (pszRet == nullptr) {
        if (isEOF()) return 0;  // legit end of file
        return HResult::GetDef(GetStreamError(), HRESULT_WIN32_C(ERROR_READ_FAULT));
    }

    if (_iLineNumCur >= 0) _iLineNumCur++;
    return StrT::Len(ret.get_PtrConst());  // Return length in chars.
}

bool cFileText::put_TextPos(const cTextPos& rPos) {
    // FILE_BEGIN == SEEK_SET
    if (!rPos.isValidPos()) return false;
    const HRESULT hRes = SeekX(rPos.get_Offset(), SEEK_t::_Set);
    if (FAILED(hRes)) return false;
    _iLineNumCur = rPos.get_LineNum();
    return true;
}
#endif
}  // namespace Gray
