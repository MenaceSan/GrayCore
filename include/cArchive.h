//! @file cArchive.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
#ifndef _INC_cArchive_H
#define _INC_cArchive_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif
#include "cStream.h"

namespace Gray {
/// <summary>
/// a bidirectional (typeless) binary stream of serialized data. Use with cObject.
/// @note this is inherently dangerous to use since it contains no default/automatic typing/versioning information.
/// @note put cVariant into the archive if you desire typing information. (and some version change resistance)
/// This is extensible to any type.
/// Similar to the MFC CArchive type. except &lt;&lt; &gt;&gt; are overridden by the type of the Archive (store vs retrieve).
/// i.e. store and retrieve a particular structure can use the same code.
/// </summary>
class GRAYCORE_LINK cArchive {
    const bool m_bStoring;         /// What mode is this in? true = writing to the m_pStream cStreamOutput. else reading cStreamInput.
    cStreamBase* const m_pStream;  /// cStreamInput or cStreamOutput depending on m_bStoring

 public:
    cArchive(cStreamOutput& so) noexcept : m_bStoring(true), m_pStream(&so) {}
    cArchive(cStreamInput& si) noexcept : m_bStoring(false), m_pStream(&si) {}
    cArchive(cStream& s, bool bStoring) noexcept : m_bStoring(bStoring), m_pStream(bStoring ? static_cast<cStreamBase*>(static_cast<cStreamOutput*>(&s)) : static_cast<cStreamBase*>(static_cast<cStreamInput*>(&s))) {}

    /// <summary>
    /// I am storing the object to the write archive cStreamOutput. like MFC cArchive
    /// </summary>
    inline bool IsStoring() const noexcept {
        return m_bStoring;
    }

    /// <summary>
    /// I am loading the object from the read archive cStreamInput. like MFC cArchive
    /// </summary>
    inline bool IsLoading() const noexcept {
        return !m_bStoring;
    }

    cStreamOutput& ref_Out() {
        ASSERT(IsStoring());
        return *static_cast<cStreamOutput*>(m_pStream);
    }
    cStreamInput& ref_Inp() {
        ASSERT(IsLoading());
        return *static_cast<cStreamInput*>(m_pStream);
    }

    /// Serialize Base Types
    HRESULT Serialize(void* pData, size_t nSize);
    HRESULT SerializeSize(size_t& nSize);

    HRESULT Write(const void* pData, size_t nSize) {
        // Emulate MFC. Insert into the archive.
        ASSERT(IsStoring());
        return Serialize(const_cast<void*>(pData), nSize);
    }
    HRESULT Read(void* pData, size_t nSize) {
        // Emulate MFC. Extract from the archive.
        ASSERT(IsLoading());
        return Serialize(pData, nSize);
    }

    /// <summary>
    /// Emulate MFC.
    /// </summary>
    /// <returns>size_t</returns>
    COUNT_t ReadCount() {
        ASSERT(IsLoading());
        size_t n;
        SerializeSize(n);
        return n;
    }
    /// <summary>
    /// Emulate MFC.
    /// </summary>
    void WriteCount(size_t n) {
        ASSERT(IsStoring());
        SerializeSize(n);
    }

    // define operators for serialization of a TYPE.
#define CTYPE_DEF(a, _TYPE, c, d, e, f, g, h)                              \
    HRESULT Serialize(_TYPE& Val) { return Serialize(&Val, sizeof(Val)); } \
    cArchive& operator<<(const _TYPE& Val) {                               \
        Write(&Val, sizeof(Val));                                          \
        return *this;                                                      \
    }                                                                      \
    cArchive& operator>>(_TYPE& Val) {                                     \
        Read(&Val, sizeof(Val));                                           \
        return *this;                                                      \
    }

#include "cTypes.tbl"
#undef CTYPE_DEF
};
}  // namespace Gray

#ifndef _MFC_VER  // emulate MFC.
typedef Gray::cArchive CArchive;
// #define cArchive Gray::cArchive
#define DECLARE_SERIAL(class_name) friend cArchive& GRAYCALL operator>>(cArchive& ar, class_name*& pOb);  //   _DECLARE_DYNCREATE(class_name)
#define IMPLEMENT_SERIAL(class_name, base_name, quant)                                                    // external to class.
#endif
#endif
