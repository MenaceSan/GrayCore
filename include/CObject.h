//! @file cObject.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cObject_H
#define _INC_cObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMem.h"
#include "cOSHandle.h"  // HMODULE

namespace Gray {
class cArchive;

/// <summary>
/// use this to make sure 2 DLL/SO's agree on the ABI packing/format and version of some object.
/// </summary>
template <UINT32 _SIGVALID = 0xCA11AB1e>
class DECLSPEC_NOVTABLE cObjectSignature : public cMemSignature<_SIGVALID> {
    typedef cMemSignature<_SIGVALID> SUPER_t;

    const UINT32 m_nVer;         /// Must be agreed to by all users. e.g. _INC_GrayCore_H
    const size_t _nSizeOfParent;  /// Must be agreed to by all users. sizeof(PARENTCLASS) for checking alignments/packing of structures (ABI).

 public:
    cObjectSignature(UINT32 nVer, size_t nSizeOfParent) : m_nVer(nVer), _nSizeOfParent(nSizeOfParent) {}
    ~cObjectSignature() {
        ASSERT(isValidSignature());
    }

    /// <summary>
    /// Call this from the context of some other DLL or lib and make sure they match.
    /// If not, then there are un-matching pound defines (conditional code) or different compiler packing params. This is BAD.
    /// Make sure this is inline compiled.
    /// </summary>
    /// <param name="nVer"></param>
    /// <param name="nSizeOfParent"></param>
    /// <returns></returns>
    bool inline IsValidSignature(UINT32 nVer, size_t nSizeOfParent) const noexcept {
        if (!SUPER_t::isValidSignature()) return false;
        if (nVer != m_nVer || nSizeOfParent != _nSizeOfParent) return false;
        return true;
    }
};

/// <summary>
/// Emulate the MFC CObject base class. https://docs.microsoft.com/en-us/cpp/mfc/reference/cobject-class?view=msvc-160
/// Generic base class of all stuff. May be used to replace/emulate _CPPRTTI?
/// Implements Archiving (SErialization) and RTTI.
/// May be base for stack or heap allocated object.
/// </summary>
#ifndef _MFC_VER
class GRAYCORE_LINK CObject {
 public:
    virtual ~CObject() {}
};

#ifdef _DEBUG
#define ASSERT_VALID(p) (p)->AssertValid()  // Emulate MFC
#else
#define ASSERT_VALID(p) ASSERT((p) != nullptr)  // Cheaper call in release mode.
#endif
#endif  // _MFC_VER

/// <summary>
/// Similar to MFC CObject but with more function. isValidCheck.
/// </summary>
struct GRAYCORE_LINK cObject : public CObject {
    /// <summary>
    /// get my vtable/vftable/vfptr that is attached to my HMODULE
    /// </summary>
    const void* const* GetVTable() const;

    ::HMODULE get_HModule() const noexcept;

    /// Emulate MFC method. cArchive = CArchive. BUT has HRESULT return instead of void and throw.
    virtual HRESULT Serialize(cArchive& a);

    /// <summary>
    /// use with AssertValid()
    /// ASSUME ! cMem::IsCorruptApp()
    /// NOT in MFC so use COBJECT_IsValidCheck to call.
    /// @note This can't be called in constructors and destructor of course !
    /// </summary>
    /// <returns></returns>
    virtual bool isValidCheck() const noexcept {  /// memory allocation and structure definitions are valid.
        if (!cMem::IsValidApp(this)) return false;
        if (!IsValidCast<const cObject>(this)) {  // structure definitions are valid. use _CPPRTTI.
            DEBUG_CHECK(false);
            return false;
        }
        return true;
    }
    virtual void AssertValid() const {  /// memory allocation and structure definitions are valid.
        //! MFC override = virtual void AssertValid() const;
        ASSERT(isValidCheck());
    }

    /// <summary>
    /// Safer external check for valid object does not use the vtable immediately/directly!
    /// </summary>
    /// <param name="p"></param>
    /// <returns></returns>
    static bool IsValidCheck(const cObject* p) noexcept {
        return !cMem::IsCorruptApp(p, sizeof(cObject)) && p->isValidCheck();  // check stuff but no ASSERT
    }
};

#ifndef _MFC_VER
// Dynamic cObject is one that can be created knowing only its name and perhaps some interface that it supports. using cObjectFactoryT<T>
#define DECLARE_DYNAMIC(c)        //__noop
#define DECLARE_DYNCREATE(c)      //__noop. cWinClassT, cObjectFactory
#define IMPLEMENT_DYNAMIC(c, cb)  //__noop
#endif                            // _MFC_VER
}  // namespace Gray
#endif
