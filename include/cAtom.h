//! @file cAtom.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cAtom_H
#define _INC_cAtom_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "HResult.h"
#include "cString.h"

namespace Gray {
typedef HASHCODE32_t ATOMCODE_t;  /// Hash a (case ignored) atom as a 32 bit hashcode instead of using its name/pointer. using StrT::GetHashCode32().

#define CATOM_STR(a) a        /// Part of a static atom quoted string. for concatenate use. e.g. "Tag_%s"
#define CATOM_CAT(a, b) a##b  /// https://gcc.gnu.org/onlinedocs/cpp/Concatenation.html#Concatenation
#define CATOM_N(a) #a         /// A static atom i know is defined some place, but i just want to use the string here. Typically used by property bag. (e.g."SymName","Root")

/// <summary>
/// A single string name shared by all.
/// Similar to the _WIN32 ATOM GlobalAddAtom(). but not system shared of course.
/// case independent. e.g. "THIS"=="this"=>same atom.
/// commonly used atoms should be constructed at startup/init time: e.g. static const cAtomRef a_Root("Root");
/// nullptr == "" == no atom.
/// </summary>
struct GRAYCORE_LINK cAtomRef : public cRefPtr<cStringHeadT<ATOMCHAR_t>> {
    friend struct cAtomManager;
    typedef cStringA STR_t;
    typedef cAtomRef THIS_t;
    typedef cStringHeadT<ATOMCHAR_t> HEAD_t;
    typedef cRefPtr<HEAD_t> SUPER_t;

 private:
    static cAtomRef GRAYCALL FindorCreateAtom(const cSpan<ATOMCHAR_t>& src) noexcept;
    static cAtomRef GRAYCALL FindorCreateAtom(const STR_t& sText) noexcept;

 public:
    explicit cAtomRef(const HEAD_t* pDef = nullptr) noexcept  // cAtomManager only.
        : SUPER_t(pDef) {
        // nullptr = no atom. == "". in cAtomManager. FindArgForKey, etc.
    }
    cAtomRef(const THIS_t& ref) noexcept : SUPER_t(ref) {}  // copy and inc ref.
    cAtomRef(const STR_t& sName) noexcept : THIS_t(FindorCreateAtom(sName)) {}
    cAtomRef(const cSpan<ATOMCHAR_t>& sp) noexcept : THIS_t(FindorCreateAtom(sp)) {
        //! @note cAtomRef can be defined in the C++ init code. So the cAtomManager must be safe to run at init time.
    }
    cAtomRef(const ATOMCHAR_t* pszName) noexcept : THIS_t(StrT::ToSpanStr(pszName)) {}

    bool isValidCheck() const noexcept {
        return isValidPtr() && get_Ptr()->isValidString();
    }
    bool IsEmpty() const noexcept {
        return !isValidPtr();   // There is no such thing as an empty atom/string
    }

    void SetEmptyA();
    ~cAtomRef() {
        SetEmptyA();
    }

    /// <summary>
    /// Every user of the atom earns 1/n of the usage of the memory
    /// </summary>
    /// <param name="iAllocCount"></param>
    /// <returns></returns>
    size_t CountHeapStats(OUT ITERATE_t& iAllocCount) const;

    /// <summary>
    /// particular hash value is not important. Value just needs to be unique and consistent.
    /// </summary>
    ATOMCODE_t get_HashCode() const noexcept {
        return isValidPtr() ? get_Ptr()->get_HashCode() : k_HASHCODE_CLEAR;
    }

    /// <summary>
    /// Get cStringA
    /// </summary>
    STR_t get_StrA() const noexcept {
        return STR_t(get_Ptr());
    }

    const ATOMCHAR_t* get_CPtr() const noexcept {  /// as a C string
        return isValidPtr() ? get_Ptr()->get_PtrConst() : &STR_t::k_Nil;
    }
    operator const ATOMCHAR_t*() const noexcept {  /// as a C string
        return get_CPtr();
    }
    StrLen_t GetLength() const noexcept {
        return isValidPtr() ? get_Ptr()->get_CharCount() : 0;
    }

    cSpan<ATOMCHAR_t> get_SpanStr() const noexcept {
        return isValidPtr() ? get_Ptr()->get_SpanStr() : cSpan<ATOMCHAR_t>();
    }
    operator cSpan<ATOMCHAR_t>() const {
        return get_SpanStr();
    }

    bool IsEqualNoCase(const ATOMCHAR_t* pStr) const noexcept {
        return isValidPtr() ? get_Ptr()->IsEqualNoCase(pStr) : StrT::IsNullOrEmpty(pStr);
    }
    bool operator==(const ATOMCHAR_t* pStr) const noexcept {
        return IsEqualNoCase(pStr);
    }
    inline bool operator==(const THIS_t& m2) const noexcept {
        return get_HashCode() == m2.get_HashCode();
    };

    const THIS_t& operator=(const THIS_t& atom) {
        if (!IsEqual(atom)) {
            SetEmptyA();
            put_Ptr(atom);
        }
        return *this;
    }
    const THIS_t& operator=(const ATOMCHAR_t* pStr) {
        if (!IsEqualNoCase(pStr)) {
            SetEmptyA();
            put_Ptr(FindorCreateAtom(StrT::ToSpanStr(pStr)));
        }
        return *this;
    }
    const THIS_t& operator=(const STR_t& sStr) {
        if (!IsEqualNoCase(sStr)) {
            SetEmptyA();
            put_Ptr(FindorCreateAtom(sStr));
        }
        return *this;
    }

    /// <summary>
    /// Make this atom permanent. never removed from the atom table.
    /// </summary>
    void SetAtomStatic();

    /// <summary>
    /// Find the atom in the atom table ONLY if it exists. DO NOT CREATE!
    /// </summary>
    /// <param name="pszText"></param>
    /// <returns>Empty atom if not found.</returns>
    static cAtomRef GRAYCALL FindAtomStr(const ATOMCHAR_t* pszText);

    /// <summary>
    /// Get this hash id if a valid atom hash exists. DO NOT CREATE!
    /// </summary>
    /// <param name="idAtom"></param>
    /// <returns>Empty atom if not found.</returns>
    static cAtomRef GRAYCALL FindAtomHashCode(ATOMCODE_t idAtomCode);

    /// <summary>
    /// Filter the string and make a legal symbolic name using only valid symbol characters.
    /// </summary>
    /// <param name="symName">OUT the identifier (valid char set) out to this buffer. ASSUME k_LEN_MAX_CSYM</param>
    /// <param name="pszExp">source string</param>
    /// <param name="bAllowDots">allow dots and stuff to follow them. JSON</param>
    /// <returns>Length of the tag + args. 0 = nothing here worth doing anything with. -lt- 0 = error.</returns>
    static StrLen_t GRAYCALL MakeSymName(cSpan<ATOMCHAR_t> symName, const ATOMCHAR_t* pszExp, bool bAllowDots = false);

#ifdef _DEBUG
    /// <summary>
    /// Dump all the atoms to a file.
    /// </summary>
    /// <param name="pszFilePath"></param>
    /// <returns></returns>
    static HRESULT GRAYCALL DebugDumpFile(const FILECHAR_t* pszFilePath);
#endif
};
}  // namespace Gray
#endif  // _INC_cAtom_H
