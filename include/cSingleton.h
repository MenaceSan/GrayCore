//! @file cSingleton.h
//!	A singleton is a type of class of which only one single instance may exist.
//! This is commonly used for management classes used to control system-wide resources.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cSingleton_H
#define _INC_cSingleton_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cDebugAssert.h"
#include "cDependRegister.h"
#include "cHeapObject.h"
#include "cNonCopyable.h"
#include "cObject.h"
#include "cThreadLock.h"
#include "cTypeInfo.h"

namespace Gray {
#ifdef _MSC_VER
#pragma warning(disable : 4355)  // disable the warning regarding 'this' pointers being used in base member initializer list. Singletons rely on this action
#endif

// NON template base / class for all singletons.
class GRAYCORE_LINK cSingletonBase : public cObject {
    DECLARE_cNonCopyable(cSingletonBase);

 protected:
    cSingletonBase() {}

 public:
    static cLockerT<cThreadLockableFast> GetLockAll();  /// lock all use of singletons to a thread.
};

/// <summary>
/// base class for a type where only one instance can exist at a time in the process.
/// @note TYPE = cSingletonType based class = this
/// Externally created singleton pointer. might be stack based, or abstract (e.g.cNTServiceImpl) but usually static allocated. e.g. g_MyName global.
/// @note Assume 1. gets constructed/destructed by the C Runtime, 2. Is inherently thread safe since its not created on demand.
/// @note The BIG problem with this is that we cannot guarantee order of creation/destruction. So singletons that rely/construct on each other may be corrupt/uninitialized.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingletonType : public cSingletonBase {
    static TYPE* sm_pThe;  /// pointer to the one and only object of this TYPE. ASSUME automatically init to = nullptr.

 protected:
    TYPE* get_This() {
        return static_cast<TYPE*>(this);  // cast to TYPE. ASSUME same as sm_pThe.
    }
    void ClearSing() {
        sm_pThe = nullptr;
    }

 protected:
    /// <summary>
    /// the singleton must be constructed with a reference to top level object. Probably the same as 'this' but might not be in multi-inherit case.
    /// typically this == pThis == sm_pThe
    /// </summary>
    /// <param name="pThis"></param>
    cSingletonType(TYPE* pThis) noexcept {
        DEBUG_ASSERT(pThis != nullptr, "cSingletonType");
        if (sm_pThe != nullptr) {
            // THIS SHOULD NOT HAPPEN! Find who else is creating this singleton!
            DEBUG_ASSERT(sm_pThe == nullptr, "cSingletonType");
            return;
        }
        sm_pThe = pThis;
        DEBUG_ASSERT(sm_pThe == get_This(), "cSingletonType");  // is it really always the same?? IS this a waste of time/code ?
    }
    virtual ~cSingletonType() {
        // ASSUME if isInCExit() this will be called AFTER cDependRegister has done its cleanup!!
        if (sm_pThe != nullptr) {
            DEBUG_ASSERT(sm_pThe == get_This(), "~cSingletonType");
            sm_pThe = nullptr;
        }
    }

 public:
    /// has this singleton been created yet? may be effected by static init time load order. or destruct order?
    static inline bool isSingleCreated() noexcept {
        return sm_pThe != nullptr;
    }
    /// <summary>
    /// get sm_pThe unchecked. allow that it might be nullptr. Weird case.
    /// </summary>
    /// <returns></returns>
    static inline TYPE* get_SingleU() noexcept {
        return sm_pThe;
    }

    /// <summary>
    /// Get pointer to singleton. ASSUME this object exists.
    /// if static type = This is a complex or abstract or assumed that we cannot just create automatically on first usage.
    /// </summary>
    static inline TYPE* get_Single() noexcept {
        DEBUG_CHECK(isSingleCreated());  // assume already created.
        return sm_pThe;                  // get_SingleU()
    }
    /// <summary>
    /// Assume derived class will override I() or get_Single() to create and init object on first reference.
    /// </summary>
    static inline TYPE& I() noexcept {
        return *get_Single();
    }

    /// <summary>
    /// Some singletons can have a derived type. e.g. cAppImpl
    /// MUST be based on global static!
    /// </summary>
    /// <typeparam name="TYPE2"></typeparam>
    /// <returns></returns>
    template <class TYPE2>
    static inline TYPE2* GRAYCALL get_SingleCast() {  // ASSUME TYPE2 derived from TYPE?
        return PtrCastCheck<TYPE2>(get_Single());
    }
};

template <class TYPE>
TYPE* cSingletonType<TYPE>::sm_pThe = nullptr;  // assume nullptr init will ALWAYS work regardless of static init usage order.

// 1. a singleton that is defined by a global variable.
// like : DECLARE_DYNAMIC
#define DECLARE_cSingletonGlobal(TYPE)  // do nothing.

// 2. a singleton that is created via a locally defined static variable. use cSingletonStatic_IMPL()
template <class T>
using cSingletonStatic = cSingletonType<T>;

#define DECLARE_cSingletonStatic(TYPE)          \
    static TYPE& GRAYCALL I() noexcept;         \
    static inline TYPE* get_Single() noexcept { \
        return &I();                            \
    }

/// This MUST be declared in the HMODULE that we want associated with the singleton. the implementation of the class. like : IMPLEMENT_DYNAMIC
#define cSingletonStatic_IMPL(TYPE)     \
    TYPE& GRAYCALL TYPE::I() noexcept { \
        static TYPE sm_The;             \
        return sm_The;                  \
    }

// 3. a dynamic allocated singleton.
/// <summary>
/// abstract base class for singleton created lazy/on demand if it does not yet exist.
/// Thread safe. ALWAYS mark derived class as 'final'
/// ASSUME cDependRegister will handle proper destruct order on app close or module unload.
/// ASSUME TYPE is based on cSingleton and IHeapObject.
/// @note This can get created at C static init time if used inside some other static. But later is OK too of course.
///  It's Safe being constructed INSIDE another C runtime init constructor. (order irrelevant) ASSUME m_pThe = nullptr at init.
///  http://www.cs.wustl.edu/~schmidt/editorial-3.html
/// get (or create) a pointer to the derived singleton object.
/// similar to CreateObject from cObjectFactory Like the MFC CreateObject() and "CRuntime?"
/// @note This ensures proper creation order for singletons (Statics) that ref each other!
/// @note This can create a race condition. This decides the true TYPE of the object.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingleton : public cSingletonType<TYPE>, public cDependRegister, public cHeapObject {
    typedef cSingletonType<TYPE> SUPER_t;
    typedef cSingleton<TYPE> THIS_t;
    DECLARE_cHeapObject(THIS_t);

 protected:
    /// <summary>
    /// construct a singleton. typically this == pThis is type of cSingleton.
    /// </summary>
    /// <param name="pThis"></param>
    /// <param name="rAddrCode">typeid(XXX) but GCC doesn't like it as part of template? track the module for the codes implementation.</param>
    cSingleton(TYPE* pThis, const TYPEINFO_t& rAddrCode = typeid(TYPE)) noexcept : SUPER_t(pThis), cDependRegister(rAddrCode) {}

    /// <summary>
    /// i created this cHeapObject so i know there are no other refs. OR maybe uses cRefBase so cDependRegister cant just delete it?
    /// </summary>
    /// <returns></returns>
    bool isReferenced() const noexcept override {
        return false;
    }
};

// extend def of derived type class.
#define DECLARE_cSingleton(TYPE)                 \
    static TYPE* GRAYCALL get_Single() noexcept; \
    static inline TYPE& I() noexcept {           \
        return *get_Single();                    \
    }                                            \
    DECLARE_cHeapObject(TYPE);

/// This MUST be declared in the HMODULE that we want associated with the singleton. the implementation of the class.
/// Double Check Lock for multi threaded safety.  // Register only when fully constructed TYPE.
#define cSingleton_IMPL(TYPE)                      \
    TYPE* GRAYCALL TYPE::get_Single() noexcept {   \
        if (!isSingleCreated()) {                  \
            const auto guard(GetLockAll());   \
            if (!isSingleCreated()) {              \
                (new TYPE())->RegisterSingleton(); \
            }                                      \
        }                                          \
        return cSingletonType<TYPE>::get_Single(); \
    }
}  // namespace Gray
#endif  // _INC_cSingleton_H
