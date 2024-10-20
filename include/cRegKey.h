//! @file cRegKey.h
//! Wrap a handle to a windows/WIN32 registry key.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cRegKey_H
#define _INC_cRegKey_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "HResult.h"
#include "cBits.h"
#include "cHandlePtr.h"
#include "cIniBase.h"  // IIniBaseGetter
#include "cSpan.h"

#ifdef _WIN32

#ifndef HKEY_LOCAL_MACHINE  /// Not defined in normal 16 bit API
#define HKEY_LOCAL_MACHINE ((HKEY)(ULONG_PTR)((LONG)0x80000002))
#endif  // HKEY_LOCAL_MACHINE
#ifndef HKEY_CURRENT_USER
#define HKEY_CURRENT_USER ((HKEY)(ULONG_PTR)((LONG)0x80000001))
#endif  // HKEY_LOCAL_MACHINE

namespace Gray {
typedef DWORD REGVAR_t;  /// Variant data type for WIN32 registry values. e.g. REG_NONE, REG_SZ, REG_DWORD, REG_BINARY

/// <summary>
/// Bind a hard name to the default HKEY values.
/// </summary>
struct cRegKeyPath {
    ::HKEY _hKeyBase;               /// base/predef key handle. e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, HKEY_USERS
    const FILECHAR_t* _pszRegPath;  /// e.g. _FN("SOFTWARE\\Menasoft"), nullptr = use previous in array.

    /// <summary>
    /// is this a base predefined HKEY_* key? e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE.
    /// https://learn.microsoft.com/en-us/windows/win32/sysinfo/predefined-keys
    /// </summary>
    /// <param name="hKey"></param>
    /// <returns></returns>
    static inline bool IsKeyPredef(::HKEY hKey) noexcept {
        return cBits::HasAny(CastN(UINT_PTR, hKey), CastN(UINT_PTR, HKEY_CLASSES_ROOT));
    }
};

/// <summary>
/// Declare a static starting/init value for a registry/config key.
/// </summary>
struct cRegKeyInit {
    cRegKeyPath _KeyPath;
    const FILECHAR_t* _pszKeyName;  /// value name. can be nullptr (default) or _FN("MyValue")
    const void* _pValue;           /// (union) May be a string pointer or DWORD (inline). depends on _nType
    REGVAR_t _nType;                /// REG_SZ, REG_DWORD, omitted for default key. (since default must always be a string)

    bool isEndMarker() const noexcept {
        return _KeyPath._hKeyBase == HANDLEPTR_NULL;  // end of an array of values.
    }
    bool isRegValue() const noexcept {
        // ASSUME _nType is set as well.
        return _pszKeyName != nullptr;
    }
};

template <>
inline void CloseHandleType(::HKEY h) noexcept {
    //! ASSUME IsValidHandle(h)
    if (cRegKeyPath::IsKeyPredef(h)) return;  // never close predef keys
    ::RegCloseKey(h);                       // ignored BOOL return.
}

/// <summary>
/// read and write to/from the Windows registry hive by its handle. advapi32.dll
/// Similar to the MFC/ATL CRegKey. default = HKEY_LOCAL_MACHINE. ::ATL::CRegKey
/// TODO support IIniBaseSetter and IIniBaseGetter ?
/// See cRegKeyX for more function.
/// @note Key names are not case sensitive.
/// </summary>
struct GRAYCORE_LINK cRegKey : public cHandlePtr<::HKEY>, public IIniBaseGetter {
    typedef cHandlePtr<::HKEY> SUPER_t;

    static const cRegKeyPath k_aPredefNames[];  /// map predef HKEY values to names.

    cRegKey(::HKEY hKey = HKEY_LOCAL_MACHINE) noexcept : SUPER_t(hKey) {
        //! hKey = HKEY_LOCAL_MACHINE default
        //! not HKEY_CLASSES_ROOT
    }
    ~cRegKey() noexcept {}

    /// <summary>
    /// is it a base HKEY_* predefined key? e.g. HKEY_CLASSES_ROOT, HKEY_LOCAL_MACHINE
    /// </summary>
    /// <returns></returns>
    inline bool isKeyPredef() const noexcept {
        return cRegKeyPath::IsKeyPredef(get_Handle());
    }

    bool isKeyOpen() const noexcept {
        //! was RegOpenKeyEx() used ? is close required?
        if (isKeyPredef()) return false;
        return SUPER_t::isValidHandle();
    }

    HKEY DetachHandle() noexcept {
        //! like SUPER_t::DetachHandle() but use HKEY_LOCAL_MACHINE not NULL
        const ::HKEY h = get_Handle();
        ref_Handle() = HKEY_LOCAL_MACHINE;
        return h;
    }

    static const FILECHAR_t* GRAYCALL GetNamePredef(::HKEY hKeyBase) noexcept;

    const FILECHAR_t* get_NameBase() const noexcept {
        return GetNamePredef(get_Handle());
    }

    /// <summary>
    /// Open the key for reading (typically). Don't create it if not exist.
    /// @note _h = will always be changed!!! set to cOSHandle::kNULL on fail
    /// </summary>
    /// <param name="hKeyBase"></param>
    /// <param name="pszSubKey"></param>
    /// <param name="samDesired">KEY_READ, KEY_QUERY_VALUE, KEY_ALL_ACCESS (if i want to delete tags?)</param>
    /// <returns>0 = S_OK, 2=ERROR_FILE_NOT_FOUND</returns>
    HRESULT Open(::HKEY hKeyBase, const FILECHAR_t* pszSubKey, ::REGSAM samDesired = KEY_READ) {
        CloseHandle();
        const LSTATUS lRet = _FNF(::RegOpenKeyEx)(hKeyBase, pszSubKey, 0, samDesired, &ref_Handle());
        return HResult::FromWin32(lRet);
    }

    /// <summary>
    /// Open the key for writing. Create pszSubKey if it does not exist.
    /// @note function creates all missing keys in the specified path.
    /// </summary>
    /// <param name="hKeyBase">HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER</param>
    /// <param name="pszSubKey"></param>
    /// <param name="dwOptions">REG_OPTION_NON_VOLATILE</param>
    /// <param name="samDesired">0</param>
    /// <param name="pSa">the security attributes to be assigned to the new key i am creating.</param>
    /// <returns>0 = S_OK, 1=S_FALSE=was already existing.
    ///		E_INVALIDARGS = you used samDesired in place of dwOptions
    ///		HRESULT_WIN32_C(ERROR_INVALID_HANDLE)
    /// </returns>
    HRESULT OpenCreate(::HKEY hKeyBase, const FILECHAR_t* pszSubKey, DWORD dwOptions = REG_OPTION_NON_VOLATILE, ::REGSAM samDesired = KEY_ALL_ACCESS, ::SECURITY_ATTRIBUTES* pSa = nullptr) {
        DWORD dwDisposition = 0;
        // bool bRetried = false;	do_retry:
        CloseHandle();
        const LSTATUS lRet = _FNF(::RegCreateKeyEx)(hKeyBase, pszSubKey, 0,
                                                    nullptr,  // class
                                                    dwOptions, samDesired, pSa, &ref_Handle(),
                                                    &dwDisposition  // pointer to return disposition.
        );
        if (lRet == NO_ERROR) {
            return (dwDisposition == REG_OPENED_EXISTING_KEY) ? S_FALSE : S_OK;
        }
        return HResult::FromWin32(lRet);
    }

    HRESULT OpenBase(const FILECHAR_t* pszSubKey, ::REGSAM samDesired = KEY_READ) {
        //! open sub key from base key. Replaces get_HKey which is usually a base.
        ASSERT(isKeyPredef());
        return Open(get_Handle(), pszSubKey, samDesired);
    }

    HRESULT FlushX() noexcept {
        const LSTATUS lRet = ::RegFlushKey(get_Handle());
        return HResult::FromWin32(lRet);
    }

    // Keys

    /// <summary>
    /// Delete a single key by name.
    /// @note This is for keys not values. delete values using RegDeleteValue.
    /// this does not delete subkeys, use DeleteKeyTree().
    /// </summary>
    /// <param name="pszSubKey"></param>
    /// <returns>0 = S_OK, 2=ERROR_FILE_NOT_FOUND</returns>
    HRESULT DeleteKey(const FILECHAR_t* pszSubKey) noexcept {
        const LSTATUS lRet = _FNF(::RegDeleteKey)(get_Handle(), pszSubKey);
        return HResult::FromWin32(lRet);
    }

    /// <summary>
    /// Walk the list of child keys by name for a registry key.
    ///
    /// </summary>
    /// <param name="dwIndex"></param>
    /// <param name="name">the max size of the buffer and the size of the name returned.</param>
    /// <returns>0 = S_OK, or ERROR_NO_MORE_ITEMS = no more entries.</returns>
    HRESULT EnumKey(DWORD dwIndex, cSpanX<FILECHAR_t>& name) const noexcept {
        DWORD dwSizeNameMax = name.get_MaxLen();
        const LSTATUS lRet = _FNF(::RegEnumKeyEx)(get_Handle(), dwIndex, name.get_PtrWork(), &dwSizeNameMax, nullptr, nullptr, nullptr, nullptr);
        if (lRet != S_OK) return HResult::FromWin32(lRet);
        return CastN(HRESULT, dwSizeNameMax);
    }

    // Values

    /// <summary>
    /// Walk the list of values for a registry key.
    /// @note strings will always be of type FILECHAR_t
    /// </summary>
    /// <param name="dwIndex"></param>
    /// <param name="name"></param>
    /// <param name="pdwTypeRet"></param>
    /// <param name="pDataRet"></param>
    /// <param name="pdwSizeData"></param>
    /// <returns>length of name, else HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS) = no more entries. (0x80070103)</returns>
    HRESULT EnumValue(DWORD dwIndex, cSpanX<FILECHAR_t>& name, REGVAR_t* pdwTypeRet = nullptr, void* pDataRet = nullptr, DWORD* pdwSizeData = nullptr) const noexcept {
        DWORD dwSizeNameMax = name.get_MaxLen();
        const LSTATUS lRet = _FNF(::RegEnumValue)(get_Handle(), dwIndex, name.get_PtrWork(), &dwSizeNameMax, nullptr, pdwTypeRet, (LPBYTE)pDataRet, pdwSizeData);
        if (lRet != S_OK) return HResult::FromWin32(lRet);
        return CastN(HRESULT, dwSizeNameMax);
    }
    HRESULT DeleteValue(const FILECHAR_t* pszSubKey) noexcept {
        //! @return 0=S_OK, 2=ERROR_FILE_NOT_FOUND
        const LSTATUS lRet = _FNF(::RegDeleteValue)(get_Handle(), pszSubKey);
        return HResult::FromWin32(lRet);
    }
    /// <summary>
    /// Raw Write. REG_SZ must include size for '\0'.
    /// @note strings will always be of type FILECHAR_t
    /// </summary>
    /// <param name="pszValueName">nullptr = default value for the key.</param>
    /// <param name="dwType">REGVAR_t</param>
    /// <param name="pData"></param>
    /// <param name="dwDataSize"></param>
    /// <returns>0 = S_OK</returns>
    HRESULT SetValue(const FILECHAR_t* pszValueName, REGVAR_t dwType, const cMemSpan& data) noexcept {
        const LSTATUS lRet = _FNF(::RegSetValueEx)(get_Handle(), pszValueName, 0, dwType, data.GetTPtrC<BYTE>(), CastN(DWORD, data.get_SizeBytes()));
        return HResult::FromWin32(lRet);
    }

    /// <summary>
    /// Registry Raw Read.
    /// @note strings will always be of type FILECHAR_t
    /// </summary>
    /// <param name="pszValueName">key name. nullptr = default key.</param>
    /// <param name="rdwType">REGVAR_t REG_DWORD, REG_SZ, etc.</param>
    /// <param name="pData">nullptr = just return the size we need.</param>
    /// <param name="rdwDataSize"></param>
    /// <returns>0 = S_OK, ERROR_FILE_NOT_FOUND</returns>
    HRESULT QueryValue(const FILECHAR_t* pszValueName, OUT REGVAR_t& rdwType, cMemSpan ret) const noexcept {
        DWORD dwDataSize = CastN(DWORD, ret.get_SizeBytes());
        const LSTATUS lRet = _FNF(::RegQueryValueEx)(get_Handle(), pszValueName, nullptr, &rdwType, ret.GetTPtrW(), &dwDataSize);
        if (lRet) return HResult::FromWin32(lRet);
        return CastN(HRESULT, dwDataSize);
    }

    /// <summary>
    /// set DWORD value to the registry value.
    /// </summary>
    /// <param name="lpszValueName"></param>
    /// <param name="dwValue"></param>
    /// <returns>0 = S_OK</returns>
    HRESULT SetValueDWORD(const FILECHAR_t* lpszValueName, DWORD dwValue) {
        ASSERT_NN(lpszValueName);
        return SetValue(lpszValueName, REG_DWORD, TOSPANT(dwValue));
    }

    // Helper Combos
    HRESULT OpenQuerySubKey(::HKEY hKeyBase, const FILECHAR_t* pszSubKey, cSpanX<FILECHAR_t> ret) {
        //! This is always a string type data.
        const HRESULT hRes = Open(hKeyBase, pszSubKey, KEY_QUERY_VALUE);
        if (FAILED(hRes)) return hRes;
        REGVAR_t dwType = REG_SZ;  // always REG_SZ or REG_EXPAND_SZ
        return QueryValue(nullptr, OUT dwType, ret);
    }

    /// <summary>
    /// Convert Registry value of any type to a String. Non reversible.
    /// </summary>
    /// <param name="bExpand">Use ::ExpandEnvironmentStrings() for stuff like "%PATH%"</param>
    /// <returns></returns>
    static StrLen_t GRAYCALL MakeValueStr(cSpanX<FILECHAR_t> ret, REGVAR_t dwType, const cMemSpan& data, bool bExpand = false);

    /// <summary>
    /// Get a string from a registry value regardless of its actual type. convert to a string if needed.
    /// </summary>
    /// <param name="lpszValueName"></param>
    /// <param name="pszValue"></param>
    /// <param name="iSizeMax">sizeof(pszValue);</param>
    /// <param name="bExpand"></param>
    /// <returns> 0 = S_OK</returns>
    HRESULT QueryValueStr(const FILECHAR_t* lpszValueName, cSpanX<FILECHAR_t> ret, bool bExpand = false) const;

    HRESULT PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const override;
};
}  // namespace Gray
#endif
#endif
