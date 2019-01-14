//
//! @file CTempPool.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//
#ifndef _INC_CStrTemp_H
#define _INC_CStrTemp_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CArray.h"
#include "CHeap.h"
#include "CThreadLocalSys.h"

namespace Gray
{
	class GRAYCORE_LINK CTempPool
	{
		//! @class Gray::CTempPool
		//! A set of thread safe temporary strings/spaces for function arguments and Unicode/UTF8 conversions. Used by StrArg<>.
		//! Pool of re-used strings/spaces after k_iCountMax uses.
		//! use a new set for each thread. Thread Local/Safe.
		//! @note This is a bit of a hack as it assumes the strings are not in use when the rollover occurs !
		//!  beware of using more than k_iCountMax strings on one line.
		//!  We can never be sure we are not re-using strings before they are ready.
		//!  Just use cString is you want to always be safe?

	public:
		int m_iCountCur;	//!< rotate this count to re-use buffers in m_aBlocks.
		CArrayVal2<CHeapBlock> m_aBlocks;	//!< Temporary blocks to be used on a single thread.

		static const int k_iCountMax = 16;	//!< Assume nested functions won't use more than m_aBlocks in a single thread. (e.g. This is the max number of args on a single sprintf)

		//! Allow this to be overloaded with a version that destructs on thread close.
		static IThreadLocal* sm_pThreadLocal;	// can use CThreadLocalTypeNew instead. 
		static CThreadLocalSysNew<CTempPool> sm_ThreadLocalDefault;	// default for sm_pThreadLocal. set sm_pThreadLocal with CThreadLocalTypeNew in more strict applications.

	public:
		CTempPool()
		: m_iCountCur(0)
		{
		}
		virtual ~CTempPool()
		{
		}

		void CleanTemps();
		void* GetTempV(size_t nLenNeed);		// get void/bytes.
		void* GetTempV(size_t nLenNeed, const void* pData);

		template< typename TYPE>
		inline TYPE* GetTempT(StrLen_t nLenNeed)
		{
			//! @arg nLenNeed = will add a space for '\0'
			return (TYPE*)(GetTempV((nLenNeed + 1) * sizeof(TYPE)));
		}
		template< typename TYPE>
		inline TYPE* GetTempT(StrLen_t nLenNeed, const TYPE* pData)
		{
			//! @arg nLenNeed = will add a space for '\0'
			return (TYPE*)(GetTempV((nLenNeed + 1) * sizeof(TYPE), pData));
		}

		static CTempPool* GRAYCALL GetTempPool();
		static void GRAYCALL FreeTempsForThreadManually();

		static void* GRAYCALL GetTempSV(size_t nLenNeed, const void* pData) // static
		{
			//! Get thread local temp space.
			return GetTempPool()->GetTempV(nLenNeed,pData);
		}

		template< typename TYPE>
		static TYPE* GRAYCALL GetTempST(StrLen_t nLenNeed)
		{
			//! Get thread local temp space.
			//! @arg nLenNeed = will add a space for '\0'
			return GetTempPool()->GetTempT<TYPE>(nLenNeed);
		}
		template< typename TYPE>
		static TYPE* GRAYCALL GetTempST(StrLen_t nLenNeed, const TYPE* pData)
		{
			//! Get thread local temp space.
			//! @arg nLenNeed = will add a space for '\0'
			return GetTempPool()->GetTempT<TYPE>(nLenNeed, pData);
		}
	};
};
#endif
