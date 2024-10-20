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
    const bool _isStoring;         /// What mode is this in? true = writing to the _pStream cStreamOutput. else reading cStreamInput.
    cStreamBase* const _pStream;  /// cStreamInput or cStreamOutput depending on _isStoring

 public:
    cArchive(cStreamOutput& so) noexcept : _isStoring(true), _pStream(&so) {}
    cArchive(cStreamInput& si) noexcept : _isStoring(false), _pStream(&si) {}
    cArchive(cStream& s, bool bStoring) noexcept : _isStoring(bStoring), _pStream(bStoring ? static_cast<cStreamBase*>(static_cast<cStreamOutput*>(&s)) : static_cast<cStreamBase*>(static_cast<cStreamInput*>(&s))) {}

    /// <summary>
    /// I am storing the object to the write archive cStreamOutput. like MFC cArchive
    /// </summary>
    inline bool IsStoring() const noexcept {
        return _isStoring;
    }

    /// <summary>
    /// I am loading the object from the read archive cStreamInput. like MFC cArchive
    /// </summary>
    inline bool IsLoading() const noexcept {
        return !_isStoring;
    }

    cStreamOutput& ref_Out() {
        ASSERT(IsStoring());
        return *static_cast<cStreamOutput*>(_pStream);
    }
    cStreamInput& ref_Inp() {
        ASSERT(IsLoading());
        return *static_cast<cStreamInput*>(_pStream);
    }

    /// Serialize Base Types

    /// <summary>
    /// Serialize this to cMemSpan
    /// </summary>
    /// <param name="ret">cMemSpan</param>
    /// <returns>-lt- 0 = error HRESULT_WIN32_C(ERROR_IO_INCOMPLETE)</returns>
    HRESULT Serialize(cMemSpan ret);

    /// <summary>.
    /// Write a compressed size. high bit of byte is reserved to say there is more to come. bytes stored low to high (Intel endian of course).
    /// MFC calls this "Count"
    /// </summary>
    /// <param name="nSize"></param>
    /// <returns></returns>
    HRESULT SerializeSize(size_t& nSize);

    template <typename _TYPE>
    HRESULT SerializeT(_TYPE& val) {
        return Serialize(TOSPANT(val));
    } 

    //******************************************
    // dont use stuff below here ! DONT Emulate MFC.

    HRESULT Write(const void* pData, size_t nSize) {
        // Emulate MFC. Insert into the archive.
        ASSERT(IsStoring());
        return Serialize(cMemSpan(pData, nSize));
    }
    HRESULT Read(void* pData, size_t nSize) {
        // Emulate MFC. Extract from the archive.
        ASSERT(IsLoading());
        return Serialize(cMemSpan(pData, nSize) );
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
    template <typename _TYPE>
    cArchive& operator<<(const _TYPE& val) {    
        Write(&val, sizeof(val));    
        return *this;                           
    }                                           
    template <typename _TYPE>
    cArchive& operator>>(_TYPE& val) {                                    
        Read(&val, sizeof(val));                               
        return *this;                                                     
    }
};
}  // namespace Gray

#ifndef _MFC_VER  // emulate MFC.
typedef Gray::cArchive CArchive;
// #define cArchive Gray::cArchive
#define DECLARE_SERIAL(class_name) friend cArchive& GRAYCALL operator>>(cArchive& ar, class_name*& pOb);  //   _DECLARE_DYNCREATE(class_name)
#define IMPLEMENT_SERIAL(class_name, base_name, quant)                                                    // external to class.
#endif
#endif
