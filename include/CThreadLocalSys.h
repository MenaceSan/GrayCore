//! @file cThreadLocalSys.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cThreadLocalSys_H
#define _INC_cThreadLocalSys_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "PtrCast.h"

#ifdef __linux__
#include <pthread.h>                                      // pthread_key_t
typedef void(NTAPI* PFLS_CALLBACK_FUNCTION)(IN void* p);  // like FARPROC

#elif defined(_WIN32) && defined(__GNUC__)  // __GNUC__ didnt define this !
typedef VOID(NTAPI* PFLS_CALLBACK_FUNCTION)(PVOID lpFlsData);
WINBASEAPI DWORD WINAPI FlsAlloc(PFLS_CALLBACK_FUNCTION lpCallback);  // replace TlsAlloc()
WINBASEAPI BOOL WINAPI FlsFree(DWORD dwFlsIndex);
WINBASEAPI PVOID WINAPI FlsGetValue(DWORD dwFlsIndex);
WINBASEAPI BOOL WINAPI FlsSetValue(DWORD dwFlsIndex, PVOID lpFlsData);
#endif

namespace Gray {

/// <summary>
/// Store a (void*) value separate/local for each thread.
/// @note Must manually supply PFLS_CALLBACK_FUNCTION thread destructor for this type else pointer leaks!
/// @note can't get data for thread other than current! NO GetDataForThreadId
/// similar to MFC cThreadLocalObject cThreadLocal
/// similar to C++11 https://en.cppreference.com/w/cpp/keyword/thread_local
/// </summary>
class cThreadLocalSys {
 public:
#ifdef _WIN32  // ASSUME (_WIN32_WINNT >= 0x0600)
#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES 0xffffffff  // TLS_OUT_OF_INDEXES not defined on UNDER_CE
#endif
    typedef DWORD TYPESLOT_t;  /// from ::FlsAlloc() or ::TlsAlloc() if (_WIN32_WINNT < 0x0600)

#elif defined(__linux__)
#define TLS_OUT_OF_INDEXES ((pthread_key_t)-1)
#define NTAPI
    typedef pthread_key_t TYPESLOT_t;  /// from ::pthread_key_create
#else
#error NOOS
#endif

 private:
    TYPESLOT_t m_nTypeSlot;  /// id for the type of data stored per thread. if (_WIN32_WINNT >= 0x0600) ::FlsAlloc(), etc.

 public:
    /// <summary>
    /// Allocate new (void*) to be stored for EACH thread. Associate this type with m_nTypeSlot
    /// </summary>
    /// <param name="pDestruct">supply a destructor if i think i need one when a thread is destroyed. (e.g. delete)</param>
    cThreadLocalSys(::PFLS_CALLBACK_FUNCTION pDestruct = nullptr) noexcept {
#ifdef _WIN32
        // Use (newer) ::FlsAlloc (has destructor) over old XP specific ::TlsAlloc. limited to 128 uses.
        m_nTypeSlot = ::FlsAlloc(pDestruct);
#elif defined(__linux__)
        int iRet = ::pthread_key_create(&m_nTypeSlot, pDestruct);
        if (iRet != 0) {
            m_nTypeSlot = TLS_OUT_OF_INDEXES;  // failed for some reason.
        }
#endif
        DEBUG_CHECK(isInit());
    }

    ~cThreadLocalSys() {
        DEBUG_CHECK(isInit());
#ifdef _WIN32
        ::FlsFree(m_nTypeSlot);
#elif defined(__linux__)
        int iRet = ::pthread_key_delete(m_nTypeSlot);
        ASSERT(iRet == 0);
        UNREFERENCED_PARAMETER(iRet);
#endif
    }
    bool isInit() const noexcept {
        //! Before static init?
#ifdef _WIN32
        if (m_nTypeSlot == 0) return false;
#endif
        if (m_nTypeSlot == TLS_OUT_OF_INDEXES) return false;
        return true;
    }

    /// <summary>
    /// If PutData was called on this thread, Get info stored for the current thread.  from LPVOID
    /// Same as MFC::cThreadLocal:GetData()
    /// </summary>
    /// <returns></returns>
    void* GetData() const noexcept {
        if (!isInit()) {
            // DEBUG_CHECK(0);
            return nullptr;  // Before static init!
        }
#ifdef _WIN32
        return ::FlsGetValue(m_nTypeSlot);
#elif defined(__linux__)
        return ::pthread_getspecific(m_nTypeSlot);
#endif
    }

    bool PutData(void* pData) noexcept {
        //! Store something unique to this thread. from LPVOID
        DEBUG_CHECK(isInit());  // Before static init!
#ifdef _WIN32
        return ::FlsSetValue(m_nTypeSlot, pData) ? true : false;
#elif defined(__linux__)
        int iRet = ::pthread_setspecific(m_nTypeSlot, pData);
        return iRet == 0;
#endif
    }
};

/// <summary>
/// template typed version of cThreadLocalSys
/// @note if TYPE needs a destructor call then i must supply it via pDestruct.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
struct cThreadLocalSysT : public cThreadLocalSys {
    typedef cThreadLocalSys SUPER_t;
    cThreadLocalSysT(::PFLS_CALLBACK_FUNCTION pDestruct = nullptr) noexcept : cThreadLocalSys(pDestruct) {
        STATIC_ASSERT(sizeof(TYPE*) <= sizeof(void*), cThreadLocalSysT);  // ?
    }
    TYPE* GetData() const noexcept {
        return (TYPE*)SUPER_t::GetData();
    }
    bool PutData(TYPE* nData) noexcept {
        return SUPER_t::PutData((void*)nData);
    }
};

/// <summary>
/// like cThreadLocalSysT but with auto create/allocate/new TYPE if it doesn't already exist.
/// Will delete pointer when thread closes.
/// </summary>
/// <typeparam name="TYPE"></typeparam>
template <class TYPE>
class cThreadLocalSysNew : public cThreadLocalSysT<TYPE> {
    typedef cThreadLocalSysT<TYPE> SUPER_t;

 protected:
    static void NTAPI OnThreadClose(IN void* pData) {
        //! The thread has closed (or cThreadLocalSys was destroyed) so destroy/free/delete my TYPE pointer object.
        ASSERT_NN(pData);
        TYPE* pData2 = PtrCast<TYPE>(pData);
        delete pData2;
    }

 public:
    cThreadLocalSysNew() noexcept : cThreadLocalSysT<TYPE>(OnThreadClose) {}

    /// <summary>
    /// Create new if not yet exist. like GetData()
    /// </summary>
    TYPE* GetDataNew() {
        TYPE* pData = SUPER_t::GetData();
        if (pData == nullptr) {
            pData = new TYPE;
            SUPER_t::PutData(pData);
        }
        return pData;
    }

    /// Manually free. reverse of GetDataNewV
    void FreeDataManually() {
        TYPE* pData = SUPER_t::GetData();
        if (pData != nullptr) {
            SUPER_t::PutData(nullptr);
            delete pData;
        }
    }
};
}  // namespace Gray
#endif
