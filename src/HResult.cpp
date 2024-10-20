//! @file HResult.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "HResult.h"
#include "StrBuilder.h"
#include "cLogMgr.h"
#include "cPair.h"
#include "cFilePath.h"
#include "cFileText.h"

#ifndef UNDER_CE
#include <errno.h>
#endif

namespace Gray {
static cArrayPtr<const HResultCode> s_HResult_CodeSets;  // local private Facility_t sets . Must call HResult::AddCodesDefault();

const HResult::Facility_t HResult::k_Facility[] = {
    // Names of Known facilities
    Facility_t(FACILITY_POSIX, _GT("Posix")),
    Facility_t((FACILITY_TYPE)FACILITY_WIN32, _GT("Win32")),
    Facility_t((FACILITY_TYPE)FACILITY_INTERNET, _GT("Internet")),
    Facility_t((FACILITY_TYPE)FACILITY_HTTP, _GT("Http")),
    Facility_t(FACILITY_MMSYS, _GT("MMSys")),
    Facility_t(FACILITY_D3D, _GT("D3D")),  // DirectX

    Facility_t((FACILITY_TYPE)FACILITY_DISPATCH, _GT("Dispatch")),
    Facility_t((FACILITY_TYPE)FACILITY_ITF, _GT("ITF")),
    Facility_t((FACILITY_TYPE)FACILITY_SECURITY, _GT("Security")),
    Facility_t((FACILITY_TYPE)FACILITY_COMPLUS, _GT("ComPlus")),

    Facility_t((FACILITY_TYPE)FACILITY_NULL, nullptr),  // end
};

int HResultCode::FindCode(HRESULT hRes) const {
    for (int i = 0; this[i]._pszMsg != nullptr; i++) {
        if (this[i]._nCode == hRes) return i;
    }
    return k_ITERATE_BAD;
}

HRESULT GRAYCALL HResult::FromPOSIX(int iErrno) noexcept {
#ifndef UNDER_CE
    switch (iErrno) {
        case 0:  // EZERO
            return S_OK;
        case ENOENT:  // 2 =  A directory component in pathname does not exist or is a dangling symbolic link.
            return HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND);
        case EACCES:  // The parent directory does not allow write permission to the process, or one of the directories in pathname did not allow search permission. (See also path_resolution(2).)
            return HRESULT_WIN32_C(ERROR_ACCESS_DENIED);
        case EBADF:  // (Bad file number) fputs with bad FILE ?
            return HRESULT_WIN32_C(ERROR_INVALID_HANDLE);
        case EPERM:  // kill() not allowed
            // case EPERM: //  The file system containing pathname does not support the creation of directories.
            return HRESULT_WIN32_C(ERROR_PRIVILEGE_NOT_HELD);
        case EEXIST:  // pathname already exists (not necessarily as a directory). This includes the case where pathname is a symbolic link, dangling or not.
            return HRESULT_WIN32_C(ERROR_ALREADY_EXISTS);
        case ENOMEM:  //   Insufficient kernel memory was available.
            return HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY);
        case EROFS:  // pathname refers to a file on a read-only file system.
            return HRESULT_WIN32_C(ERROR_WRITE_FAULT);
#ifdef __linux__
        case EWOULDBLOCK:  // same value as case EAGAIN
#else
        case EAGAIN:
#endif
            return HRESULT_WIN32_C(WSAEWOULDBLOCK);
#ifdef __linux__
        case ECONNRESET:                              // "Connection reset by peer"
            return HRESULT_WIN32_C(WSAECONNRESET);    // bad close.
        case ENOTCONN:                                // "Transport endpoint is not connected"
        case ECONNABORTED:                            // "Software caused connection abort"
            return HRESULT_WIN32_C(WSAECONNABORTED);  // normal close.
        case EPIPE:                                   // "Broken Pipe"
            return HRESULT_WIN32_C(WSAECONNABORTED);
        case EADDRINUSE:  // = 98 = "Cant Bind. Address already in use"
            return HRESULT_WIN32_C(WSAEADDRINUSE);
#endif
        case EINVAL:
            return E_INVALIDARG;
        case ENOTDIR:  //  A component used as a directory in pathname is not, in fact, a directory.
            return HRESULT_WIN32_C(ERROR_PATH_NOT_FOUND);
        case E2BIG:  // Argument list (which is system dependent) is too big.
            return TYPE_E_OUTOFBOUNDS;
        case EFAULT:  // pathname points outside your accessible address space.
            return E_POINTER;
        case ENAMETOOLONG:  //  pathname was too long.
            return HRESULT_WIN32_C(ERROR_BUFFER_OVERFLOW);
        case ENOEXEC:  // Command-interpreter file has invalid format and is not executable.
                       // ERROR_BAD_EXE_FORMAT
#if 0
		case ELOOP: //  Too many symbolic links were encountered in resolving pathname.
		case ENOSPC: //  The device containing pathname has no room for the new directory.
		case ENOSPC: //  The new directory cannot be created because the user's disk quota is exhausted.
#endif
            break;
    }
#endif

    // pack a posix code in HRESULT
    return Make(FACILITY_POSIX, (iErrno)&0x0000FFFF);
}

#if !defined(UNDER_CE)
HRESULT GRAYCALL HResult::GetPOSIXLast() noexcept {  // static
    //! Get last POSIX error code. 'errno' converted to HRESULT
#if USE_CRT
    return FromPOSIX(errno);
#else
    return GetLast();
#endif
}
#endif

HRESULT GRAYCALL HResult::GetLast() noexcept {  // static
    //! Get the last system error recorded for this thread.
    //! match against HRESULT_WIN32_C(x)
#ifdef _WIN32
    const DWORD dwLastError = ::GetLastError();  // Maybe FACILITY_WIN32 or already HRESULT
    return FromWin32(dwLastError);
#elif defined(__linux__)
    // errno = EAGAIN, etc.
    return GetPOSIXLast();
#else
#error NOOS
#endif
}

void GRAYCALL HResult::SetLast(HRESULT h) {  // static
#ifdef _WIN32
    ::SetLastError(h);
#endif
}

//******************************************************************

void GRAYCALL HResult::AddCodes(const HResultCode* pCodes) {  // static
    //! Add a block of custom HResult codes, usually for a particular FACILITY_TYPE
    //! enable HResult::GetTextV()
    if (s_HResult_CodeSets.HasArg3(pCodes)) return;  // ignore pointer dupes. // already loaded

    // TODO
    // Is sorted?? test.
    s_HResult_CodeSets.Add(pCodes);
}

void GRAYCALL HResult::AddCodesDefault() {  // static
    //! configure error text for normal system errors.
    //! static library must pull this code intentionally.
    //! @todo get rid of these in favor of dynamic loading of s_HResult_CodeSets from file.
    // A _WIN32 code packed in HRESULT. Some are not handled by FormatMessage(). WinINet for instance.

    static bool s_Loaded = false;
    if (s_Loaded) return;
    s_Loaded = true;

    static const HResultCode k_CodesWin32[] = {
    /// Known codes in FACILITY_WIN32
    // @todo move text to a separate file that we optionally read.
#define HRESULT_WIN32_DEF(a, b, c) {HRESULT_WIN32_C(a), _AT(c)},
#include "HResultWin32.tbl"
#undef HRESULT_WIN32_DEF
        {S_OK, nullptr},  // terminated.
    };
    AddCodes(k_CodesWin32);

    static const HResultCode k_CodesOther[] = {
    /// Known codes NOT in FACILITY_WIN32
    // @todo move text to a separate file that we optionally read.
#define HRESULT_ENTRY(a, b, c, d) {a, _AT(d)},
#include "HResults.tbl"
#undef HRESULT_ENTRY
        {S_OK, nullptr},  // terminated.
    };
    AddCodes(k_CodesOther);
}

HRESULT GRAYCALL HResult::AddCodesText(const char* pszText) {  // static
    //! add a block of codes (and text) from some text blob (that i parse).
    //! Lines of comma separated text.
    UNREFERENCED_PARAMETER(pszText);
    // TODO
    return 0;
}

HRESULT GRAYCALL HResult::AddCodesFile(const FILECHAR_t* pszFilePath) {  // static
    cFileTextReader fileReader;
    const HRESULT hRes = fileReader.OpenX(pszFilePath, OF_READ | OF_TEXT | OF_CACHE_SEQ);
    if (FAILED(hRes)) return hRes;

    // TODO

    return E_NOTIMPL;
}

const char* GRAYCALL HResult::GetTextBase(HRESULT hRes) {  // static
    //! Get raw unformatted text for HRESULT codes from s_HResult_CodeSets first
    //! ASSUME we called AddCodesDefault() already.

    if (hRes == S_OK) return "OK";  // This is never an error.

#ifndef GRAY_STATICLIB
    HResult::AddCodesDefault();  // Since we load this anyhow make sure we are using HResult::AddCodesDefault();
#endif

    for (const HResultCode* pCodes : s_HResult_CodeSets) {
        const int j = pCodes->FindCode(hRes);
        if (j >= 0) return pCodes[j]._pszMsg;
    }

#if defined(__linux__)
    const FACILITY_TYPE eFacility = GetFacility(hRes);
    if (eFacility == FACILITY_POSIX) {  // its a POSIX code?
        const DWORD dwErrorCode = GetCode(hRes);
        const GChar_t* pszMsg = ::strerror(dwErrorCode);
        if (pszMsg != nullptr) return pszMsg;
    }
#endif

    // No idea. ASSUME FromWin32() was called if necessary.
    return nullptr;
}

bool GRAYCALL HResult::GetTextSys(HRESULT hRes, StrBuilder<GChar_t>& sb, const void* pSource, va_list vargs) {  // static
#ifdef _WIN32
    // FORMAT_MESSAGE_ALLOCATE_BUFFER
    DWORD dwFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    if (pSource != nullptr) {  // not FORMAT_MESSAGE_FROM_STRING?
        dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    if (vargs != k_va_list_empty) {
        dwFlags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
    }

    // nLenRet = number of TCHARs
    cSpanX<GChar_t> spanWrite(sb.GetSpanWrite(cFilePath::k_MaxLen));

    const DWORD nLenRet = _GTN(::FormatMessage)(dwFlags,
                                                pSource,  // use with FORMAT_MESSAGE_FROM_STRING or FORMAT_MESSAGE_FROM_HMODULE
                                                hRes,
                                                LANG_NEUTRAL,  // MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US)
                                                spanWrite.get_PtrWork(), spanWrite.get_MaxLen(), (vargs != k_va_list_empty) ? &vargs : nullptr);
    if (nLenRet > 0) {
        // successful translation -- trim any trailing junk
        sb.AdvanceWrite(nLenRet);
        sb.SetTrimWhiteSpaceEnd();
        return true;
    }
#endif

    // Not a system known code? check k_Facility.
    const FACILITY_TYPE eFacility = GetFacility(hRes);
    const DWORD dwErrorCode = GetCode(hRes);  // LSTATUS/error_status_t

    for (const HResult::Facility_t& fac : k_Facility) {
        if (fac.get_A() == eFacility) {
            // show the (known) facility name and sub code (in facility).
            sb.AddStr(fac.get_B());
            sb.AddStr(_GT(" Code "));
            sb.AddInt(dwErrorCode);
            return true;
        }
    }

    return false;  // no system code description avail.
}

void GRAYCALL HResult::GetTextV(HRESULT hRes, StrBuilder<GChar_t>& sb, const void* pSource, va_list vargs) {  // static
    // a known error code?
    // First test my internal message (if i have one).
    const char* pszErrorBase = GetTextBase(hRes);
    if (!StrT::IsNullOrEmpty(pszErrorBase)) {
        sb.WriteString(pszErrorBase);
    } else if (!GetTextSys(hRes, sb, pSource, vargs)) {  // else try system codes.
        // no facility code? default just show the error number in hex.
        sb.AddStr(_GT("Error Code "));
    }
    // Append the full code to the text.
    sb.AddStr(_GT(" ("));
    sb.AddUInt((UINT)hRes, 0x10);
    sb.AddChar(')');
}

HRESULT GRAYCALL HResult::GetHResFromStr(const cSpan<GChar_t>& str) {  // static
    //! Reverse lookup of error string.
    //! given a string from GetTextV() get the original HRESULT code.
    //! @return HRESULT Error code from a string.

    StrLen_t nLenError = str.get_MaxLen() - 1;
    if (nLenError <= 1) return S_FALSE;
    const GChar_t* pszError = str;

    // "Error Code 0%x" for unknown codes.
    // "%s Code %d" for facility and code.
    // or ends in " (0%x)" for full message and code.
    bool bHasEndParen = (pszError[nLenError] == ')');
    if (bHasEndParen) {
        nLenError--;
    }
    int iDigits = 0;
    for (; nLenError > 0 && StrChar::IsDigitRadix(pszError[nLenError], 0x10); nLenError--) {
        iDigits++;
    }
    if (iDigits <= 0 || nLenError <= 1) return S_FALSE;

    const GChar_t* pszErrorCode = pszError + nLenError + 1;
    if (nLenError == 10 && !StrT::CmpN(pszError, _GT("Error Code "), 11)) {        
        return CastN(HRESULT, StrT::toU(pszErrorCode)); // full code.
    }
    if (nLenError > 6 && !StrT::CmpN(pszErrorCode - 6, _GT(" Code "), 6)) {
        // facility + code.
        for (size_t i = 0;; i++) {
            if (i >= _countof(k_Facility) - 1) return S_FALSE;
            if (StrT::StartsWithI(pszError, k_Facility[i].get_B())) {
                return Make(k_Facility[i].get_A(), (WORD)StrT::toU(pszErrorCode));
            }
        }
    }

    if (bHasEndParen && nLenError > 2 && !StrT::CmpN(pszErrorCode - 2, _GT(" ("), 2)) {
        // full code. hex number. 0 prefix.
        return CastN(HRESULT, StrT::toU(pszErrorCode));
    }

    return S_FALSE;  // No idea.
}
}  // namespace Gray
