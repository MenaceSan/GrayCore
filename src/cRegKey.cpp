//! @file cRegKey.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "StrArg.h"
#include "cFilePath.h"
#include "cRegKey.h"

#ifdef __GNUC__
#define HKEY_PERFORMANCE_TEXT ((HKEY)(ULONG_PTR)((LONG)0x80000050))
#define HKEY_PERFORMANCE_NLSTEXT ((HKEY)(ULONG_PTR)((LONG)0x80000060))
#endif
#if (WINVER < 0x0400)
#define HKEY_CURRENT_CONFIG ((HKEY)(ULONG_PTR)((LONG)0x80000005))
#define HKEY_DYN_DATA ((HKEY)(ULONG_PTR)((LONG)0x80000006))
#endif

#ifdef _WIN32
#pragma comment(lib, "advapi32.lib")  // RegCloseKey, etc.

namespace Gray {

const cRegKeyPath cRegKey::k_aPredefNames[] = {
    // map default HKEY values to names.
    {HKEY_CLASSES_ROOT, _FN("HKCR")},     {HKEY_CURRENT_USER, _FN("HKCU")},     {HKEY_LOCAL_MACHINE, _FN("HKLM")},       {HKEY_USERS, _FN("HKU")},
#ifndef UNDER_CE
    {HKEY_PERFORMANCE_DATA, _FN("HKPD")}, {HKEY_PERFORMANCE_TEXT, _FN("HKPT")}, {HKEY_PERFORMANCE_NLSTEXT, _FN("HKPN")}, {HKEY_CURRENT_CONFIG, _FN("HKCC")}, {HKEY_DYN_DATA, _FN("HKDD")},
#endif
};

const FILECHAR_t* GRAYCALL cRegKey::GetNamePredef(::HKEY hKeyBase) noexcept {  // static
    //! Get a text name for a base registry HKEY.
    if (!cRegKeyPath::IsKeyPredef(hKeyBase)) return nullptr;

    for (const auto& predef : k_aPredefNames) {
        if (hKeyBase == predef._hKeyBase) return predef._pszRegPath;
    }
    // NOT a valid base key.
    DEBUG_ASSERT(0, "GetNamePredef");
    return _FN("H??");
}

StrLen_t GRAYCALL cRegKey::MakeValueStr(cSpanX<FILECHAR_t> ret, REGVAR_t dwType, const cMemSpan& src, bool bExpand) {  // static
    if (ret.isEmpty()) return 0;
    switch (dwType) {
        case REG_NONE:
            break;
        case REG_SZ:
            return StrT::CopyLen(ret.get_PtrWork(), src.GetTPtrC<FILECHAR_t>(), cValT::Min(ret.get_MaxLen(), CastN(StrLen_t, src.get_SizeBytes() / sizeof(FILECHAR_t))));
        case REG_EXPAND_SZ:
#ifndef UNDER_CE
            if (bExpand) {
                FILECHAR_t szTmp[cFilePath::k_MaxLen];
                StrT::CopyLen(szTmp, src.GetTPtrC<FILECHAR_t>(), cValT::Min<StrLen_t>(CastN(StrLen_t, src.get_SizeBytes() / sizeof(FILECHAR_t)), _countof(szTmp)));
                return (StrLen_t)_FNF(::ExpandEnvironmentStrings)(szTmp, ret.get_PtrWork(), ret.get_MaxLen());
            }
#endif
            return StrT::CopyLen(ret.get_PtrWork(), src.GetTPtrC<FILECHAR_t>(), cValT::Min(ret.get_MaxLen(), CastN(StrLen_t, src.get_SizeBytes() / sizeof(FILECHAR_t))));
        case REG_BINARY: {
#if USE_UNICODE
            ASSERT(0);
            break;
#else
            return StrT::ConvertToCSV(ret, src);  // convert binary blob to string.
#endif
        }
        case REG_DWORD: {
            if (src.get_SizeBytes() < sizeof(DWORD)) break;
            const DWORD dwVal = *src.GetTPtrC<DWORD>();
            return StrT::ItoA(dwVal, ret);
        }
        case REG_DWORD_BIG_ENDIAN: {
            if (src.get_SizeBytes() < sizeof(DWORD)) break;
            const DWORD dwVal = cValT::NtoH(*src.GetTPtrC<DWORD>());
            return StrT::ItoA(dwVal, ret);
        }
        case REG_LINK:                      // Symbolic Link (UNICODE)
        case REG_MULTI_SZ:                  // Multiple Unicode strings
        case REG_RESOURCE_LIST:             // Resource list in the resource map
        case REG_FULL_RESOURCE_DESCRIPTOR:  // Resource list in the hardware description
        case REG_RESOURCE_REQUIREMENTS_LIST:
            break;
#if _MSC_VER >= 1300
        case REG_QWORD: {  // 64-bit number
            if (src.get_SizeBytes() < sizeof(UINT64)) break;
            const UINT64 qVal = *src.GetTPtrC<UINT64>();
            return StrT::ILtoA(qVal, ret);
        }
#endif
    }
    // Not sure how to convert this data!
    ret.get_PtrWork()[0] = '\0';
    DEBUG_CHECK(0);
    return 0;
}

HRESULT cRegKey::QueryValueStr(const FILECHAR_t* pszValueName, cSpanX<FILECHAR_t> ret, bool bExp) const {
    ASSERT_NN(pszValueName);
    ASSERT(!ret.isEmpty());
    REGVAR_t dwType = REG_NONE;  // REG_SZ
    const HRESULT hRes = QueryValue(pszValueName, OUT dwType, ret);
    if (FAILED(hRes)) {
        ret.get_PtrWork()[0] = '\0';
        return hRes;
    }
    return MakeValueStr(ret, dwType, cMemSpan(ret, hRes), bExp);
}

HRESULT cRegKey::PropGet(const IniChar_t* pszPropTag, OUT cStringI& rsValue) const {  // override; IIniBaseGetter
    FILECHAR_t szTmpData[StrT::k_LEN_Default];
    const HRESULT hRes = QueryValueStr(StrArg<FILECHAR_t>(pszPropTag), TOSPAN(szTmpData), false);
    if (FAILED(hRes)) return hRes;
    rsValue = szTmpData;
    return S_OK;
}
}  // namespace Gray
#endif
