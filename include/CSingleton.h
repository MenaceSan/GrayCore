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
#include "cHeapObject.h"
#include "cNonCopyable.h"
#include "cObject.h"
#include "cThreadLock.h"
#include "cTypeInfo.h"

namespace Gray {
#ifdef _MSC_VER
#pragma warning(disable : 4355)  // disable the warning regarding 'this' pointers being used in base member initializer list. Singletons rely on this action
#endif

/// <summary>
/// base class for a type where only one instance can exist at a time in the process.
/// @note TYPE = cSingletonStatic based class = this
/// Externally created singleton pointer. might be stack based, or abstract (e.g.cNTServiceImpl) but usually static allocated. e.g. g_MyName global.
/// @note Assume 1. gets constructed/destructed by the C Runtime, 2. Is inherently thread safe since its not created on demand.
/// @note The BIG problem with this is that we cannot guarantee order of creation/destruction. So singletons that rely/construct on each other may be corrupt/uninitialized.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingletonStatic {
    NonCopyable_IMPL(cSingletonStatic);

 protected:
    static TYPE* sm_pThe;  /// pointer to the one and only object of this TYPE. ASSUME automatically init to = nullptr.

    TYPE* get_This() { 
        return static_cast<TYPE*>(this);    // cast to TYPE.
    }

 protected:
    /// <summary>
    /// the singleton must be constructed with a reference to top level object. Probably the same as 'this' but might not be in multi-inherit case.
    /// typically this == pThis == sm_pThe
    /// </summary>
    /// <param name="pThis"></param>
    cSingletonStatic(TYPE* pThis) noexcept {
        DEBUG_ASSERT(pThis != nullptr, "cSingletonStatic");
        if (sm_pThe != nullptr) {
            // THIS SHOULD NOT HAPPEN! Find who else is creating this singleton!
            DEBUG_ASSERT(sm_pThe == nullptr, "cSingletonStatic");
            return;
        }
        sm_pThe = pThis;
        DEBUG_ASSERT(sm_pThe == get_This(), "cSingletonStatic");  // is it really always the same?? IS this a waste of time/code ?
    }
    virtual ~cSingletonStatic() noexcept {
        //! the singleton accessors
        if (sm_pThe != nullptr) {
            DEBUG_ASSERT(sm_pThe == get_This(), "~cSingletonStatic");
            sm_pThe = nullptr;
        }
    }

    /// <summary>
    /// Some singletons can have a derived type. e.g. cAppImpl
    /// </summary>
    /// <typeparam name="TYPE2"></typeparam>
    /// <returns></returns>
    template <class TYPE2>
    static TYPE2* GRAYCALL get_SingleCast() {  // ASSUME TYPE2 derived from TYPE?
        return PtrCastCheck<TYPE2>(get_Single());
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
    static inline TYPE* get_Single() {
        DEBUG_CHECK(isSingleCreated());
        return sm_pThe;  // get_SingleU()
    }
    /// <summary>
    /// The singleton by reference.
    /// </summary>
    /// <returns></returns>
    static inline TYPE& I() noexcept {
        return *get_Single();
    }

};

template <class TYPE>
TYPE* cSingletonStatic<TYPE>::sm_pThe = nullptr;  // assume nullptr init will ALWAYS work regardless of static init usage order.

/// <summary>
/// NON template abstract base for cSingleton. MUST be IHeapObject
/// Register this to allow for proper order of virtual destruction at C runtime destruct.
/// Allows for ordered destruction of singletons if modules unload. (Not in proper reverse load order)
/// @note Static singletons are not multi threaded anyhow. so don't worry about static init order for sm_LockSingle. assume static init is single threaded.
/// @TODO allocate static space and use linked list to store order ?
/// </summary>
class GRAYCORE_LINK cSingletonRegister : public cObject, public cHeapObject {
    friend class cSingletonManager;
    // CHEAPOBJECT_IMPL;  /// Get the top level "new" pointer in the case of multiple inheritance.

 protected:
#ifndef UNDER_CE
    ::HMODULE m_hModuleLoaded;  /// What modules loaded this ? So singletons can be destroyed if DLL/SO unloads. cOSModule::GetModuleHandleForAddr()
#endif
 public:
    static cThreadLockCount sm_LockSingle;  /// common lock for all cSingleton.

 protected:
    cSingletonRegister(const TYPEINFO_t& rAddrCode) noexcept;
    ~cSingletonRegister() noexcept override;
    void RegisterSingleton() noexcept;

    /// <summary>
    /// override this to Destroy any children i might have from some other module.
    /// </summary>
    /// <param name="hMod"></param>
    /// <returns></returns>
    virtual void ReleaseModuleChildren(::HMODULE hMod) {
        ASSERT(m_hModuleLoaded != hMod);
    }

 public:
    /// <summary>
    /// Destroy all singletons in this module if it is unloading.
    /// </summary>
    static void GRAYCALL ReleaseModule(::HMODULE hMod);
};

/// <summary>
/// abstract base class for singleton created lazy/on demand if it does not yet exist.
/// Thread safe. ALWAYS mark derived class as 'final'
/// ASSUME cSingletonRegister will handle proper destruct order on app close or module unload.
/// ASSUME TYPE is based on cSingleton and IHeapObject.
/// @note This can get created at C static init time if used inside some other static. But later is OK too of course.
///  It's Safe being constructed INSIDE another C runtime init constructor. (order irrelevant) ASSUME m_pThe = nullptr at init.
///  http://www.cs.wustl.edu/~schmidt/editorial-3.html
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cSingleton : public cSingletonStatic<TYPE>, public cSingletonRegister {
    typedef cSingletonStatic<TYPE> SUPER_t;

 protected:
    /// <summary>
    /// construct a singleton. typically this == pThis is type of cSingleton.
    /// </summary>
    /// <param name="pThis"></param>
    /// <param name="rAddrCode">typeid(XXX) but GCC doesn't like it as part of template? track the module for the codes implementation.</param>
    cSingleton(TYPE* pThis, const TYPEINFO_t& rAddrCode = typeid(TYPE)) noexcept : SUPER_t(pThis), cSingletonRegister(rAddrCode) {}
    /// <summary>
    /// I am being destroyed. sm_pThe is set to nullptr in ~cSingletonStatic
    /// </summary>
    ~cSingleton() override {}

    /// <summary>
    /// get (or create) a pointer to the derived singleton TYPE2 object.
    /// ASSUME TYPE2 derived from TYPE.
    /// similar to CreateObject from cObjectFactory Like the MFC CreateObject() and "CRuntime?"
    /// @note This ensures proper creation order for singletons (Statics) that ref each other!
    /// @note This can create a race condition. This decides the true TYPE of the object.
    /// </summary>
    /// <returns>NEVER nullptr</returns>
    static TYPE* GRAYCALL get_SingleCreate() noexcept {  // ASSUME TYPE2 derived from TYPE
        if (!isSingleCreated()) {
            // Double Check Lock for multi threaded safety.
            const auto guard(cSingletonRegister::sm_LockSingle.Lock());  // thread sync critical section.
            if (!isSingleCreated()) {
                auto p = new TYPE();
                DEBUG_CHECK(p == SUPER_t::sm_pThe);
                DEBUG_CHECK(isSingleCreated());
                p->RegisterSingleton();  // Register only when fully formed.
            }
        }
        return get_SingleU();
    }

 public:
    /// <summary>
    /// get (or create) a pointer to the singleton object.
    /// </summary>
    /// <returns></returns>
    static TYPE* GRAYCALL get_Single() noexcept {
        return get_SingleCreate();
    }

    /// <summary>
    /// The singleton by reference.
    /// </summary>
    static TYPE& GRAYCALL I() noexcept {
        return *get_SingleCreate();
    }
};

// Allow cSingleton base to call my protected/private constructor
#define SINGLETON_IMPL(T) \
    friend cSingleton<T>; \
    CHEAPOBJECT_IMPL

}  // namespace Gray
#endif  // _INC_cSingleton_H
