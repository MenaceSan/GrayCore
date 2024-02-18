//! @file cTypeInfo.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cTypeInfo_H
#define _INC_cTypeInfo_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"

#ifndef UNDER_CE
#include <typeinfo>  // type_info& typeid(class type) std::
#endif

namespace Gray {
typedef short PROPIDX_t;  /// enumerate known properties for some interface/object/class. similar to COM DISPID / MEMBERID ? allow -1 to indicate not valid.

#ifdef UNDER_CE  // no TYPEINFO_t in UNDER_CE. stub this out.
typedef struct TYPEINFO_t {
    BYTE notused;
};
#define typeid(x) 0  // stub this out.
#elif defined(_MSC_VER)
typedef ::type_info TYPEINFO_t;  // Info from typeid(TYPE)
#pragma warning(disable : 4275)  // non dll-interface class 'type_info' used as base for dll-interface class. http://msdn.microsoft.com/en-us/library/3tdb471s.aspx
#else                            // __GNUC__
typedef std::type_info TYPEINFO_t;  // Info from typeid(TYPE)
#endif

#define GETTYPEINFO(TYPE) ((const cTypeInfo&)typeid(TYPE))  //

/// <summary>
/// Get type info about a C++ class via typeid(TYPE)
/// Supplement/helper for built in TYPEINFO_t via typeid()
/// Similar to MFC CRuntimeClass, RUNTIME_CLASS()
/// @note ASSUME TYPEINFO_t/type_info Always supports name(), hashcode() ?
/// @note This is NOT natively related to vtable/vfptr at all ! NO _CPPRTTI is required. Though it is similar.
/// @todo get dynamic List of virtual members from vtable?
/// </summary>
struct GRAYCORE_LINK cTypeInfo : public TYPEINFO_t {
    /// <summary>
    /// Get a HASHCODE_t for this type.
    /// </summary>
    /// <returns></returns>
    size_t get_HashCode() const noexcept {
#ifdef _MSC_VER
        return hash_code();
#else
        return PtrCastToNum(this);
#endif
    }

    /// <summary>
    /// Get the user friendly version of the name.
    /// </summary>
    /// <returns></returns>
    const char* get_Name() const noexcept {
        return name();
    }

    /// <summary>
    /// Convert C++ name to a nicer SymName
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    static const char* GRAYCALL GetSymName(const char* name) noexcept;
    const char* get_SymName() const noexcept {
        return GetSymName(name());
    }

#ifdef _CPPRTTI
    /// <summary>
    /// get vtable/vfptr for an object allocated via new or static C object.
    /// It MUST be an object. Assume this is a 'new' returned pointer.
    /// </summary>
    /// <param name="p"></param>
    /// <returns></returns>
    static const void* const* GRAYCALL GetVTable(const void* pObject);
    static const char* GRAYCALL GetMemberName_TODO(const void** vtable, PROPIDX_t i);
#endif
};
}  // namespace Gray
#endif
