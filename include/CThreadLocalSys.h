//
//! @file cThreadLocalSys.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cThreadLocalSys_H
#define _INC_cThreadLocalSys_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"
#include "cDebugAssert.h"

#ifdef __linux__
#include <pthread.h>	// pthread_key_t
#endif

#if defined(_WIN32) && defined(__GNUC__)
typedef VOID (NTAPI *PFLS_CALLBACK_FUNCTION) ( PVOID lpFlsData );
WINBASEAPI DWORD WINAPI FlsAlloc( PFLS_CALLBACK_FUNCTION lpCallback );	// replace TlsAlloc()
WINBASEAPI BOOL WINAPI FlsFree( DWORD dwFlsIndex );
WINBASEAPI PVOID WINAPI FlsGetValue( DWORD dwFlsIndex );
WINBASEAPI BOOL WINAPI FlsSetValue( DWORD dwFlsIndex, PVOID lpFlsData );
#endif

namespace Gray
{
	DECLARE_INTERFACE(IThreadLocal)
	{
		//! @interface Gray::IThreadLocal
		//! base for a type of thread local storage. TYPE is implied.
		//! allows the system thread local type to get replaced by smarter thread locals. e.g. CThreadLocalTypeNew
		//! GetDataNewV = Get thread local value or create a new one if not already existing.
		IGNORE_WARN_INTERFACE(IThreadLocal);
		virtual void* GetDataNewV() = 0;
		//! TODO manage the life of an object. Ask for a new object, or return it when I'm done. (maybe its created or freed, maybe its cached)
	};

	class cThreadLocalSys
	{
		//! @class Gray::cThreadLocalSys
		//! Store a sizeof(void*) value separate/local for each thread.
		//! @note Must manually supply PFLS_CALLBACK_FUNCTION thread destructor for this type else pointer leaks!
		//! @note can't get data for thread other than current! NO GetDataForThreadId
		//! similar to MFC CThreadLocalObject<> CThreadLocal<>

	public:
#ifdef _WIN32
#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES 0xffffffff	// TLS_OUT_OF_INDEXES not defined on UNDER_CE
#endif
		typedef DWORD TYPESLOT_t;	//!< from ::FlsAlloc()  or ::TlsAlloc() if (_WIN32_WINNT < 0x0600)
#elif defined(__linux__)
#define TLS_OUT_OF_INDEXES 	((pthread_key_t)-1)
#define NTAPI
		typedef pthread_key_t TYPESLOT_t;	//!< from ::pthread_key_create
		typedef void (NTAPI *PFLS_CALLBACK_FUNCTION)(IN void *p);	// like FARPROC
#else
#error NOOS
#endif

	private:
		TYPESLOT_t m_nTypeSlot;		//!< id for the type of data stored per thread. if (_WIN32_WINNT >= 0x0600)

	public:
		cThreadLocalSys(PFLS_CALLBACK_FUNCTION pDestruct = nullptr)
		{
			//! Allocate new (void*) to be stored for EACH thread. Associate this type with m_nTypeSlot
			//! @arg pDestruct = supply a destructor if i think i need one when a thread is destroyed. (e.g. delete)
#ifdef _WIN32
			// Use newer FlsAlloc (has destructor) over old XP specific TlsAlloc. limited to 128 uses.
#if (_WIN32_WINNT >= 0x0600)
			m_nTypeSlot = ::FlsAlloc(pDestruct);
#else
			m_nTypeSlot = ::TlsAlloc();
#endif
#else // __linux__
			int iRet = ::pthread_key_create( &m_nTypeSlot, pDestruct);
			if (iRet!=0)
			{
				m_nTypeSlot = TLS_OUT_OF_INDEXES;	// failed for some reason.
			}
#endif
			ASSERT(isInit());
		}
		~cThreadLocalSys()
		{
			ASSERT(isInit());
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
			::FlsFree(m_nTypeSlot);
#else
			::TlsFree(m_nTypeSlot);
#endif
#else // __linux__
			int iRet = ::pthread_key_delete(m_nTypeSlot);
			ASSERT(iRet==0);
			UNREFERENCED_PARAMETER(iRet);
#endif
		}
		bool isInit() const
		{
			//! Before static init?
#ifdef _WIN32
			if (m_nTypeSlot == 0)
				return false;
#endif
			if (m_nTypeSlot == TLS_OUT_OF_INDEXES)
				return false;
			return true;
		}

		void* GetData() const
		{
			//! Get info stored for the current thread. from LPVOID
			//! Same as MFC::CThreadLocal<>:GetData()
			if (!isInit())
			{
				// DEBUG_CHECK(0);
				return nullptr;	// Before static init!
			}
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
			return ::FlsGetValue(m_nTypeSlot);
#else
			return ::TlsGetValue(m_nTypeSlot);
#endif
#else // __linux__
			return ::pthread_getspecific(m_nTypeSlot);
#endif
		}

		bool PutData(void* pData)
		{
			//! Store something unique to this thread. from LPVOID
			ASSERT(isInit()); // Before static init!
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0600)
			return ::FlsSetValue(m_nTypeSlot, pData) ? true : false;
#else
			return ::TlsSetValue(m_nTypeSlot, pData) ? true : false;
#endif
#else // __linux__
			int iRet = ::pthread_setspecific(m_nTypeSlot, pData );
			return( iRet == 0 );
#endif
		}
	};

	template <class TYPE>
	class cThreadLocalSysT : public cThreadLocalSys
	{
		//! @class Gray::cThreadLocalSysT
		//! template typed version of cThreadLocalSys
		//! @note if TYPE needs a destructor call then i must supply it via pDestruct.
		//! @note ASSUME TYPE will fit in a sizeof(void*) space.

		typedef cThreadLocalSys SUPER_t;

	public:
		cThreadLocalSysT(PFLS_CALLBACK_FUNCTION pDestruct = nullptr) noexcept
			: cThreadLocalSys(pDestruct) 
		{
			STATIC_ASSERT(sizeof(TYPE) <= sizeof(void*), cThreadLocalSysT); // ?
		}
		TYPE GetData() const // GetData
		{
			return (TYPE)SUPER_t::GetData();
		}
		bool PutData(TYPE nData)
		{
			return SUPER_t::PutData((void*)nData);
		}
	};

	template <class TYPE>
	class cThreadLocalSysNew : public cThreadLocalSysT<TYPE*>, public IThreadLocal
	{
		//! @class Gray::cThreadLocalSysNew
		//! like cThreadLocalSysT but with auto create/allocate/new TYPE if it doesn't already exist.
		//! Will delete when thread closes.
		typedef cThreadLocalSysT<TYPE*> SUPER_t;

	protected:
		static void NTAPI OnThreadClose(IN void* pData)
		{
			//! The thread has closed (or cThreadLocalSys was destroyed) so destroy/free/delete my TYPE pointer object.
			ASSERT(pData != nullptr);
			TYPE* pData2 = (TYPE*)pData;
			delete pData2;
		}

	public:
		cThreadLocalSysNew()
			: cThreadLocalSysT<TYPE*>(OnThreadClose)
		{
		}

		TYPE* GetDataNew() // GetData
		{
			//! Create new if not yet exist.
			TYPE* pData = SUPER_t::GetData();
			if (pData == nullptr)
			{
				pData = new TYPE;
				SUPER_t::PutData(pData);
			}
			return pData;
		}
		virtual void* GetDataNewV()
		{
			//! override IThreadLocal
			//! This allows smarter usage and cleanup of this TYPE.
			return GetDataNew();
		}
		void FreeDataManually()
		{
			//! Manually free. reverse of GetDataNewV
			TYPE* pData = SUPER_t::GetData();
			if (pData != nullptr)
			{
				SUPER_t::PutData(nullptr);
				delete pData;
			}
		}
	};
} 
#endif
