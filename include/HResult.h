//! @file HResult.h
//! Define HRESULT error codes and what they mean. HResult::FromPOSIX()
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_HResult_H
#define _INC_HResult_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"  // FILECHAR_t
#include "StrConst.h"
#include "cPair.h"
#include "cSpan.h"

#ifdef _WIN32
#include <winerror.h>
#endif

namespace Gray {
template <typename _TYPE_CH>
class GRAYCORE_LINK StrBuilder;  // Forward declare.

#if defined(_CPPUNWIND)
#define THROW_DEF throw()  // indicate that a function can throw exceptions.
#else
#define THROW_DEF
#endif

#ifndef SUCCEEDED  // __linux__
#define SUCCEEDED(x) (((HRESULT)(x)) >= S_OK)
#define FAILED(x) (((HRESULT)(x)) < S_OK)
#endif  // SUCCEEDED __linux__

/// <summary>
/// What general group of HRESULT error codes is this?
/// 11 bit HRESULT facility code.
/// </summary>
enum FACILITY_TYPE : WORD {
#ifdef __linux__
    FACILITY_NULL = 0,  /// Some codes produce NO facility! (e.g. E_FAIL=0x80004005L )
    FACILITY_RPC = 1,   /// Used for RPC_E_DISCONNECTED
    FACILITY_DISPATCH = 2,
    FACILITY_ITF = 4,    /// OLE_E_BLANK ??
    FACILITY_WIN32 = 7,  /// Normal windows codes. HRESULT_FROM_WIN32() or HRESULT_WIN32_C(LSTATUS/error_status_t) 0x80070XXX
                         // FACILITY_DPLAY
#endif
    FACILITY_POSIX = 5,  /// Facility for POSIX _errno in a _WIN32 style code.
#ifdef __GNUC__
    FACILITY_SECURITY = 9,   /// Normally defined in winerror.h
    FACILITY_INTERNET = 12,  /// Facility for Internet codes like 404
    FACILITY_COMPLUS = 17,
    FACILITY_HTTP = 25,  /// Facility for Internet codes like 404
    FACILITY_FVE = 49,   /// 0x31
#endif
    FACILITY_MMSYS = 0x100,  /// Facility for _WIN32 MMSYSTEM MMRESULT error codes. MMSYSERR_BASE
    // FACILITY_DXAPP		= 0x200,	/// Facility for DirectX sample application codes. Internal App codes.
    FACILITY_D3D = 0x876,  /// Facility for D3D errors. Same as _FACD3D in 'd3d9.h' . e.g. D3DERR_DEVICELOST
                           /// max = 2048 = 0x800 = 11 bits ?
};

/// <summary>
/// codes for FACILITY_WIN32. (AKA LSTATUS/error_status_t)
/// NO_ERROR = ERROR_SUCCESS = 0
/// </summary>
enum HRESULT_WIN32_TYPE_ : WORD {
#define HRESULT_WIN32_DEF(a, b, c) a = b,
#include "HResultWin32.tbl"
#undef HRESULT_WIN32_DEF
};

#if !defined(LSTATUS) && !defined(_WIN32)
typedef LONG LSTATUS;  /// AKA error_status_t. FACILITY_WIN32 codes returned from RegCreateKeyEx() etc. Maybe NOT GetLastError() since it CAN sometimes return HRESULT
#endif
#ifndef MAKE_HRESULT
#define MAKE_HRESULT(sev, fac, code) ((HRESULT)(((unsigned long)(sev) << 31) | ((unsigned long)(fac) << 16) | ((unsigned long)(code))))
#endif
#define HRESULT_WIN32_C(x) MAKE_HRESULT(1, FACILITY_WIN32, (WORD)(x))  /// a constant LSTATUS/error_status_t with no check, unlike HRESULT_FROM_WIN32()

/// <summary>
/// Other Common (non FACILITY_WIN32) HRESULT codes like E_FAIL
/// </summary>
enum HRESULT_OTHER_TYPE_ : UINT32 {
#ifdef __linux__
    S_OK = ((HRESULT)0),
    S_FALSE = ((HRESULT)1),

    // Alias/Alternate names for existing FACILITY_WIN32 codes.
    E_ACCESSDENIED = HRESULT_WIN32_C(ERROR_ACCESS_DENIED),    // "General access denied error"
    E_HANDLE = HRESULT_WIN32_C(ERROR_INVALID_HANDLE),         // "Invalid handle"
    E_OUTOFMEMORY = HRESULT_WIN32_C(ERROR_OUTOFMEMORY),       // "Ran out of memory"
    E_INVALIDARG = HRESULT_WIN32_C(ERROR_INVALID_PARAMETER),  // "One or more arguments are invalid"
#endif

#define HRESULT_ENTRY(a, b, c, d) a = MAKE_HRESULT(1, b, c),
#include "HResults.tbl"
#undef HRESULT_ENTRY
};

/// <summary>
/// Used to define a nullptr terminated table of codes (usually) for a single FACILITY_TYPE.
/// like cPair
/// ASSUME this array is HRESULT sorted.
/// </summary>
struct GRAYCORE_LINK HResultCode {
    const HRESULT _nCode;        /// DWORD error code for a FACILITY_TYPE. might just use WORD? 0 = reserved as SUCCESS.
    const char* const _pszMsg;  /// associated error message string. UTF8, nullptr = end.

    /// <summary>
    /// Find a code by its HRESULT assuming this is the first of an array of HResultCode
    /// Usually this is all of a single FACILITY_TYPE.
    /// like FindSortedARetB()
    /// @todo ASSUME codes are sorted ?
    /// </summary>
    /// <returns>-1 = not found.</returns>
    int FindCode(HRESULT hRes) const;
};

/// <summary>
/// HRESULT code processing.
/// HRESULT = (high bit=SEVERITY_ERROR, 4bit=reserve, 11bit=facility, 16bits=code)
/// AKA SCODE in old _WIN32 MFC.
/// https://msdn.microsoft.com/en-us/library/cc231198.aspx, https://en.wikipedia.org/wiki/HRESULT
/// similar to std::error_code https://en.cppreference.com/w/cpp/error/error_code
/// </summary>
struct GRAYCORE_LINK HResult {
    typedef cPair<FACILITY_TYPE, const GChar_t*> Facility_t;  /// name each FACILITY_TYPE.
    static const Facility_t k_Facility[];                     /// names of all known FACILITY_TYPE.

    const HRESULT _hRes;  // DWORD

    HResult(HRESULT hRes) noexcept : _hRes(hRes) {}
    HResult(FACILITY_TYPE eFacility, WORD wCode) noexcept : _hRes(MAKE_HRESULT(1, eFacility, wCode)) {
        //! @arg eFacility = FACILITY_TYPE = FACILITY_WIN32
        //! e.g. HRESULT_WIN32_C(WSAEACCES) = HResult(FACILITY_WIN32,WSAEACCES)
    }
    HResult(int eFacility, long wCode) noexcept : _hRes(MAKE_HRESULT(1, eFacility, wCode)) {
        //! @arg eFacility = FACILITY_TYPE = FACILITY_WIN32
        //! e.g. HRESULT_WIN32_C(WSAEACCES) = HResult(FACILITY_WIN32,WSAEACCES)
    }

    operator HRESULT() const noexcept {
        return _hRes;
    }

    /// <summary>
    /// Get just the facility sub code portion of the HRESULT. may be LSTATUS/error_status_t
    /// </summary>
    /// <param name="hRes">HRESULT_CODE(hRes) = WORD or LSTATUS</param>
    /// <returns></returns>
    static constexpr DWORD GetCode(HRESULT hRes) noexcept {
        return CastN(DWORD, hRes & 0xFFFF);
    }
    inline DWORD get_Code() const noexcept {
        return GetCode(_hRes);
    }

    static constexpr FACILITY_TYPE GetFacility(HRESULT hRes) noexcept {
        //! HRESULT_FACILITY(hRes)
        return CastN(FACILITY_TYPE, ((hRes) >> 16) & 0x1fff);
    }
    inline FACILITY_TYPE get_Facility() const noexcept {
        return GetFacility(_hRes);
    }

    constexpr static bool IsFailure(HRESULT hRes) noexcept {
        //! FAILED(hRes)
        //! like HRESULT_SEVERITY(hr)  (((hr) >> 31) & 0x1)
        return hRes < 0;
    }
    bool isFailure() const noexcept {
        //! FAILED(hRes)
        //! like HRESULT_SEVERITY(hr)  (((hr) >> 31) & 0x1)
        return _hRes < 0;
    }

    /// <summary>
    /// Make a HRESULT error code from FACILITY_TYPE + WORD code.
    /// </summary>
    /// <param name="eFacility"></param>
    /// <param name="wCode"></param>
    /// <returns></returns>
    static inline HRESULT Make(FACILITY_TYPE eFacility, WORD wCode) noexcept {
        return MAKE_HRESULT(1, eFacility, wCode);
    }

    /// <summary>
    /// Make a special HRESULT error code from FACILITY_TYPE + WORD code + bReserved.
    /// </summary>
    /// <param name="bReserved">4 for a PerfMon Code. 8=app specific error. leave 0 for normal system error code.</param>
    /// <param name="eFacility"></param>
    /// <param name="wCode"></param>
    /// <returns></returns>
    static inline HRESULT Make(BYTE bReserved, FACILITY_TYPE eFacility, WORD wCode) noexcept {
        return MAKE_HRESULT(1, eFacility, wCode) | (CastN(HRESULT, bReserved) << (16 + 11));
    }

    /// <summary>
    /// like HRESULT_FROM_WIN32(dwWin32Code) NOT HRESULT_WIN32_C(WORD)
    /// </summary>
    /// <param name="dwWin32Code">maybe LSTATUS/error_status_t or already HRESULT (see GetLastError() docs)</param>
    /// <returns></returns>
    static inline HRESULT FromWin32(DWORD dwWin32Code) noexcept {
        if (CastN(HRESULT, dwWin32Code) <= 0) {  // NO_ERROR
            // <0 shouldn't happen! supposed to be unsigned. ASSUME its already a HRESULT code.
            return CastN(HRESULT, dwWin32Code);  // already HRESULT failure. see GetLastError() docs.
        }
        if (dwWin32Code > 0xFFFF) {
            // This is weird ! NOT a proper error code !?
            dwWin32Code &= 0xFFFF;
        }
        return HRESULT_WIN32_C(dwWin32Code);
    }

#ifdef _WIN32
    /// <summary>
    /// Get HRESULT from return value from _WIN32 WaitForSingleObject(), SleepEx(), WaitForMultipleObjects()
    /// MAX of 0x80 items. STATUS_ABANDONED_WAIT_0 MAXIMUM_WAIT_OBJECTS
    /// </summary>
    /// <param name="dwRet"></param>
    /// <returns>-gt- 0 = WAIT_OBJECT_0 + i</returns>
    static inline HRESULT FromWaitRet(DWORD dwRet) noexcept {
        if (dwRet == WAIT_FAILED)  // 0xFFFFFFFF
            return E_HANDLE;
        if (dwRet == WAIT_TIMEOUT)  // 258L
            return HRESULT_WIN32_C(ERROR_WAIT_TIMEOUT);
        if (dwRet >= STATUS_ABANDONED_WAIT_0)  // no idea.
            return E_FAIL;
        // WAIT_OBJECT_0 or STATUS_WAIT_0
        return CastN(HRESULT, dwRet);  // Wait signaled done. 0=event 0
    }
#endif

    static HRESULT GRAYCALL GetLast() noexcept;
    static void GRAYCALL SetLast(HRESULT h);

    /// <summary>
    /// We know there was an error! Get code else default.
    /// If the hRes isn't an error supply a default error as hResDef.
    /// </summary>
    /// <param name="hRes"></param>
    /// <param name="hResDef"></param>
    /// <returns></returns>
    static inline HRESULT GetDef(HRESULT hRes, HRESULT hResDef = E_FAIL) noexcept {
        if (SUCCEEDED(hRes) && FAILED(hResDef)) return hResDef;  // Oddly no error was supplied! provide a default error.
        return hRes;
    }
    /// <summary>
    /// Get the last system error recorded for this thread. A known failure.
    /// If there isn't one then just use the supplied default. E_FAIL
    /// </summary>
    /// <param name="hResDef"></param>
    /// <returns></returns>
    static inline HRESULT GetLastDef(HRESULT hResDef = E_FAIL) noexcept {
        return GetDef(GetLast(), hResDef);
    }

    /// <summary>
    /// Translate/Convert a DOS/POSIX error code errno_t to HRESULT type. AKA errno
    /// POSIX calls like system() return these codes.
    /// http://www.koders.com/c/fid8425F6A15760BC96CEEEC6FD90C93E689C2C1AF5.aspx
    /// </summary>
    /// <param name="iErrno"></param>
    /// <returns></returns>
    static HRESULT GRAYCALL FromPOSIX(int iErrNo) noexcept;  // from <errno.h> style

#if !defined(UNDER_CE)
    static HRESULT GRAYCALL GetPOSIXLast() noexcept;
    static HRESULT GetPOSIXLastDef(HRESULT hResDef = E_FAIL) noexcept {  // static
        return GetDef(GetPOSIXLast(), hResDef);
    }
#endif

    static void GRAYCALL AddCodes(const HResultCode* pCodes);
    static void GRAYCALL AddCodesDefault();
    static HRESULT GRAYCALL AddCodesText(const char* pszText);
    /// <summary>
    /// add a block of codes (and text) from a text file. Lines of comma separated text.
    /// </summary>
    static HRESULT GRAYCALL AddCodesFile(const FILECHAR_t* pszFilePath);

    /// <summary>
    /// Use GetTextV.
    /// </summary>
    /// <param name="hRes"></param>
    /// <returns></returns>
    static const char* GRAYCALL GetTextBase(HRESULT hRes);

    /// <summary>
    /// Ask the system what text there is for hRes. don't call strerror() here since it has just a pointer. use GetTextBase().
    /// Use GetTextV.
    /// </summary>
    /// <param name="hRes"></param>
    /// <param name="sb"></param>
    /// <param name="pSource">use with FORMAT_MESSAGE_FROM_STRING or just FORMAT_MESSAGE_FROM_HMODULE</param>
    /// <param name="vargs"></param>
    /// <returns></returns>
    static bool GRAYCALL GetTextSys(HRESULT hRes, StrBuilder<GChar_t>& sb, const void* pSource = nullptr, ::va_list vargs = k_va_list_empty);

    /// <summary>
    /// Create a string for the error code hRes. copies error message text to a string pszError. Append numeric code to the end.
    /// similar to the __linux__ strerror()
    /// like: WIN32 has a COM API for resolving error names ?
    /// </summary>
    /// <param name="hRes"></param>
    /// <param name="sb">destination buffer</param>
    /// <param name="pSource">module handle for "pdh.dll" ?</param>
    /// <param name="vargs">additional arguments.</param>
    static void GRAYCALL GetTextV(HRESULT hRes, StrBuilder<GChar_t>& sb, const void* pSource = nullptr, ::va_list vargs = k_va_list_empty);

    static HRESULT GRAYCALL GetHResFromStr(const cSpan<GChar_t>& str);
};
}  // namespace Gray
#endif  // _INC_HResult_H
