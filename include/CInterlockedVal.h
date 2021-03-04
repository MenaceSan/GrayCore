//
//! @file cInterlockedVal.h
//! single values that are safe to change on multiple threads.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
//! @todo http://stackoverflow.com/questions/1158374/portable-compare-and-swap-atomic-operations-c-c-library

#ifndef _INC_cInterlockedVal_H
#define _INC_cInterlockedVal_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "cUnitTestDecl.h"

namespace Gray
{

#define _SIZEOF_INT 4	//! sizeof(int) seems to be 32 bits in all tested configurations. _MSC_VER and __GNUC__, 32 and 64 bit.
#if defined(USE_64BIT) && defined(__GNUC__)
#define _SIZEOF_LONG 8	//!< # bytes in long or unsigned long for __DECL_ALIGN macro. Use if we can't do sizeof(x)
#else
#define _SIZEOF_LONG 4	//!< # bytes in long or unsigned long for __DECL_ALIGN macro. Use if we can't do sizeof(x)
#endif

#if defined(_M_IX86) && (_MSC_VER >= 1000)
#pragma warning(disable:4035) // disable the no return value warning, because of assembly language
#endif

	//*************************************************************
	// 32 bit values. INT32

#if defined(_WIN32)
	//! use _WIN32 Kernel32.dll functions for 32 bit operations if we can.
	typedef LONG INTER32_t;		//!< _WIN32 API uses LONG type for 32 bit int.

	// Use intrinsic interlock if possible. _MSC_VER
	// #pragma intrinsic( _InterlockedIncrement, _InterlockedDecrement, _InterlockedExchange, _InterlockedExchangeAdd, _InterlockedCompareExchange )

#else
	// __linux__ using __GNUC__

	typedef INT32 INTER32_t;	//!< Interlock intrinsic type as INT32.

	struct __synch_xchg_INT32 { INT32 a[4]; };	// was a[100]
#define __synch_xg(x)	((struct __synch_xchg_INT32 *)(x))

	inline INT32 __cdecl InterlockedCompareExchange(INT32 VOLATILE* pDest, INT32 nValNew, INT32 nValComp) noexcept
	{
		//! Place 32 bit nValNew in *pDest if ( *pDest == nValComp )
		//! @note __linux__ only provides interlocked operations for kernel level. NOT user level!
		//! similar to wine - interlocked_cmpxchg()
		//! http://www.ibiblio.org/gfreg/ldp/GCC-Inline-Asssembly-HOWTO.html
		//! @return previous value in *pDest

#if defined(__GNUC__)
		INT32 nValPrev;

#if 0
		// from "jslock.c"
		__asm__ __volatile__
		(
			"lock\n"
			"cmpxchgl %2, (%1)\n"
			"sete %%al\n"
			"andl $1, %%eax\n"
			: "=a" (nValPrev)
			: "r" (pDest), "r" (nValNew), "a" (nValComp)
			: "cc", "memory"
		);
#endif

		__asm__ __volatile__
		(
			"lock\n"
#ifdef USE_64BIT
			"cmpxchgl %k1,%2"
#else
			"cmpxchgl %1,%2"
#endif
			: "=a"(nValPrev)
			: "q"(nValNew), "m"(*__synch_xg(pDest)), "0"(nValComp)
			: "memory"
		);
		return nValPrev;
#elif defined(_M_IX86)	// _MSC_VER 32 bit
		__asm
		{
			MOV          ecx, dword ptr[pDest]
			MOV          edx, dword ptr[nValNew]
			MOV          eax, dword ptr[nValComp]
			LOCK CMPXCHG dword ptr[ecx], edx
		}
#elif ! defined(_MT)
		// this is clearly Not thread safe but lack of _MT says I don't care.
		INT32 nValPrev = *pDest;
		if (nValPrev == nValComp)
		{
			*pDest = nValNew;
		}
		return nValPrev;
#else
#error "No implementation of InterlockedCompareExchange"
#endif
	}

	inline bool InterlockedSetIfEqual(_Inout_ INT32 VOLATILE *pDest, INT32 nValNew, INT32 nValComp) noexcept
	{
		//! most common use of InterlockedCompareExchange
		//! It's more efficient to use the z flag than to do another compare
		//! value returned in eax
		return nValComp == InterlockedCompareExchange(pDest, nValNew, nValComp);
	}
	inline INT32 __cdecl InterlockedIncrement(INT32 VOLATILE *pDest) noexcept
	{
		INT32 lValNew;
#if defined(__GNUC__)
		__asm__ __volatile__("lock; xaddl %0, %1"
			: "=r" (lValNew), "=m" (*pDest)
			: "0" (1), "m" (*pDest));
		return(lValNew + 1);
#else
		INT32 nValComp;
		do
		{
			nValComp = *pDest;
			lValNew = nValComp + 1;
		} while (!InterlockedSetIfEqual(pDest, lValNew, nValComp));
		return lValNew;
#endif
	}
	inline INT32  __cdecl InterlockedDecrement(INT32 VOLATILE *pDest) noexcept
	{
		INT32 lValNew;
#if defined(__GNUC__)
		__asm__ __volatile__("lock; xaddl %0, %1"
			: "=r" (lValNew), "=m" (*pDest)
			: "0" (-1), "m" (*pDest));
		return(lValNew - 1);
#else
		INT32 nValComp;
		do
		{
			nValComp = *pDest;
			lValNew = nValComp - 1;
		} while (!InterlockedSetIfEqual(pDest, lValNew, nValComp));
		return lValNew;
#endif
	}
	inline INT32  __cdecl InterlockedExchange(INT32 VOLATILE *pDest, INT32 Value) noexcept
	{
		INT32 nValComp;
		do
		{
			nValComp = *pDest;
		} while (!InterlockedSetIfEqual(pDest, Value, nValComp));
		return nValComp;
	}
	inline INT32  __cdecl InterlockedExchangeAdd(INT32 VOLATILE *pDest, INT32 Value) noexcept
	{
		INT32 nValComp;
		do
		{
			nValComp = *pDest;
		} while (!InterlockedSetIfEqual(pDest, nValComp + Value, nValComp));
		return nValComp;
	}
#endif // ! _WIN32

	//*************************************************************
	// 64 bit values.

#if defined(_WIN64) // (_WIN32_WINNT>=0x0502) defined(_MT) &&
	extern "C"
	{
		//! Windows has native 64 bit interlocked support,
		//! but only on Windows 2003 servers and above.
		INT64 __cdecl _InterlockedIncrement64(_Inout_ INT64 VOLATILE *pDest);
		INT64 __cdecl _InterlockedDecrement64(_Inout_ INT64 VOLATILE *pDest);
		INT64 __cdecl _InterlockedExchange64(_Inout_ INT64 VOLATILE *pDest, IN INT64 Value);
		INT64 __cdecl _InterlockedExchangeAdd64(_Inout_ INT64 VOLATILE *pDest, IN INT64 Value);
		INT64 __cdecl _InterlockedCompareExchange64(_Inout_ INT64 VOLATILE *Destination, IN INT64 ExChange, IN INT64 nValComp);
		// #pragma intrinsic( _InterlockedIncrement64, _InterlockedDecrement64, _InterlockedExchange64, _InterlockedExchangeAdd64, _InterlockedCompareExchange64 )
	} // "C"
#else

	inline INT64 __cdecl _InterlockedCompareExchange64(_Inout_ INT64 VOLATILE *pDest, IN INT64 nValNew, IN INT64 nValComp) noexcept
	{
		//! No native support for interlock 64, so i must implement it myself.
		//! Place 64 bit nValNew in *pDest if ( *pDest == nValComp )
		//! @note __linux__ only provides interlocked operations for kernel level. NOT user level!
		//! BSD/Mac = OSAtomicCompareAndSwap64Barrier()
#ifdef __GNUC__
#ifdef USE_64BIT
		INT64 nValPrev;
		__asm__ __volatile__
		(
#ifdef USE_64BIT
			"lock; cmpxchgq %1,%2"
			: "=a"(nValPrev)
			: "q"(nValNew), "m"(*__synch_xg(pDest)), "0"(nValComp)
			: "memory"
#else
			// @todo FIND 32 bit version of this 64 bit operation !!
			"lock; cmpxchg8b %2;"
			: "=a"(nValPrev)
			: "0"(nValComp), "m"(*__synch_xg(pDest)), "c"(__synch_xg(nValNew)[0]), "b"(__synch_xg(nValNew)[1])
			: "memory", "%ebx"
#endif
		);
#else
		// @todo FIND 32 bit version of this 64 bit operation !!
		INT64 nValPrev = *pDest;
		if (nValPrev == nValComp)
		{
			*pDest = nValNew;
		}
#endif
		return nValPrev;
#elif defined(_MSC_VER) && !defined(USE_64BIT)	// _MSC_VER 32 bit code for 64 bit interlock. ! USE_64BIT
		__asm
		{
			lea esi, nValComp;
			lea edi, nValNew;
			mov eax, [esi];
			mov edx, 4[esi];
			mov ebx, [edi];
			mov ecx, 4[edi];
			mov esi, pDest;
			// lock CMPXCHG8B [esi] is equivalent to the following except
			// that it's atomic:
			// ZeroFlag = (edx:eax == *esi);
			// if (ZeroFlag) *esi = ecx:ebx;
			// else edx:eax = *esi;
			lock CMPXCHG8B[esi];
		}
#elif ! defined(_MT)
		// this is clearly Not thread safe but ! _MT says I don't care.
		INT64 nValPrev = *pDest;
		if (nValPrev == nValComp)
		{
			*pDest = nValNew;
		}
		return nValPrev;
#else
#error "No implementation of InterlockedCompareExchange64"
#endif
	}

	inline bool _InterlockedSetIfEqual64(_Inout_ INT64 VOLATILE *pDest, INT64 nValNew, INT64 nValComp) noexcept
	{
		//! No native support for interlock 64, so i must implement it myself.
		//! most common use of InterlockedCompareExchange
		//! It's more efficient to use the z flag than to do another compare
		//! value returned in eax
#if defined(_MSC_VER) && !defined(USE_64BIT)	// _MSC_VER 32 bit. ! USE_64BIT
		__asm
		{
			lea esi, nValComp;
			lea edi, nValNew;
			mov eax, [esi];
			mov edx, 4[esi];
			mov ebx, [edi];
			mov ecx, 4[edi];
			mov esi, pDest;
			// lock CMPXCHG8B [esi] is equivalent to the following except
			// that it's atomic:
			// ZeroFlag = (edx:eax == *esi);
			// if (ZeroFlag) *esi = ecx:ebx;
			// else edx:eax = *esi;
			lock CMPXCHG8B[esi];
			mov eax, 0;
			setz al;
		}
#else
		return nValComp == _InterlockedCompareExchange64(pDest, nValNew, nValComp);
#endif
	}
	inline INT64 __cdecl _InterlockedIncrement64(_Inout_ INT64 VOLATILE *pDest) noexcept
	{
		INT64 nValComp;
		INT64 nValNew;
		do
		{
			nValComp = *pDest;
			nValNew = nValComp + 1;
		} while (!_InterlockedSetIfEqual64(pDest, nValNew, nValComp));
		return nValNew;
	}
	inline INT64 __cdecl _InterlockedDecrement64(_Inout_ INT64 VOLATILE *pDest) noexcept
	{
		INT64 nValComp;
		INT64 nValNew;
		do
		{
			nValComp = *pDest;
			nValNew = nValComp - 1;
		} while (!_InterlockedSetIfEqual64(pDest, nValNew, nValComp));
		return nValNew;
	}
	inline INT64 __cdecl _InterlockedExchange64(_Inout_ INT64 VOLATILE *pDest, IN INT64 Value) noexcept
	{
		INT64 nValComp;
		do
		{
			nValComp = *pDest;
		} while (!_InterlockedSetIfEqual64(pDest, Value, nValComp));
		return nValComp;
	}
	inline INT64 __cdecl _InterlockedExchangeAdd64(_Inout_ INT64 VOLATILE *pDest, IN INT64 Value) noexcept
	{
		INT64 nValComp;
		do
		{
			nValComp = *pDest;
		} while (!_InterlockedSetIfEqual64(pDest, nValComp + Value, nValComp));
		return nValComp;
	}
#endif // _WIN64

#if _MSC_VER >= 1000
#pragma warning(default:4035)
#endif

	//*************************************************************

	namespace InterlockedN
	{
		//! @namespace Gray::InterlockedN
		//! namespace for interlock templates for int 32 and 64
		//! Protected unitary operations that are safe on multi threaded/processor machines.
		//! INT32 for 32 bit, INT64 for 64 bit.
		//! @note unitary (single instruction) ops like ++ are NOT SAFE on multi CPU systems !! Tested in cThread.
		//! @note The parameters for this function must be aligned on a 32-bit boundary; otherwise, the function will behave unpredictably on multiprocessor x86 systems and any non-x86 systems. See _aligned_malloc.
		//! @note should use __DECL_ALIGN(X) to make sure we are aligned.

		template< typename TYPE >
		GRAYCORE_LINK TYPE Increment(TYPE VOLATILE* pnValue) noexcept;
		template< typename TYPE >
		GRAYCORE_LINK TYPE Decrement(TYPE VOLATILE* pnValue) noexcept;
		template< typename TYPE >
		GRAYCORE_LINK TYPE ExchangeAdd(TYPE VOLATILE* pnValue, TYPE nValue) noexcept;
		template< typename TYPE >
		GRAYCORE_LINK TYPE Exchange(TYPE VOLATILE* pnValue, TYPE nValue) noexcept;
		template< typename TYPE >
		GRAYCORE_LINK TYPE CompareExchange(TYPE VOLATILE* pnValue, TYPE nValue, TYPE lComparand) noexcept;

		// Implement 32 bits as INTER32_t
		template<> inline INTER32_t Increment<INTER32_t>(INTER32_t VOLATILE* pnValue) noexcept
		{
			return InterlockedIncrement(pnValue);
		}
		template<> inline INTER32_t Decrement<INTER32_t>(INTER32_t VOLATILE* pnValue) noexcept
		{
			return InterlockedDecrement(pnValue);
		}
		template<> inline INTER32_t ExchangeAdd<INTER32_t>(INTER32_t VOLATILE* pnValue, INTER32_t nValue) noexcept
		{
			return InterlockedExchangeAdd(pnValue, nValue);
		}
		template<> inline INTER32_t Exchange<INTER32_t>(INTER32_t VOLATILE* pnValue, INTER32_t nValue) noexcept
		{
			return InterlockedExchange(pnValue, nValue);
		}
		template<> inline INTER32_t CompareExchange<INTER32_t>(INTER32_t VOLATILE* pnValue, INTER32_t nValue, INTER32_t lComparand) noexcept
		{
			return InterlockedCompareExchange(pnValue, nValue, lComparand);
		}

		// Implement 64 bit as INT64
		template<> inline INT64 Increment<INT64>(INT64 VOLATILE* pnValue) noexcept
		{
			return _InterlockedIncrement64(pnValue);
		}
		template<> inline INT64 Decrement<INT64>(INT64 VOLATILE* pnValue) noexcept
		{
			return _InterlockedDecrement64(pnValue);
		}
		template<> inline INT64 ExchangeAdd<INT64>(INT64 VOLATILE* pnValue, INT64 nValue) noexcept
		{
			return _InterlockedExchangeAdd64(pnValue, nValue);
		}
		template<> inline INT64 Exchange<INT64>(INT64 VOLATILE* pnValue, INT64 nValue) noexcept
		{
			return _InterlockedExchange64(pnValue, nValue);
		}
		template<> inline INT64 CompareExchange<INT64>(INT64 VOLATILE* pnValue, INT64 nValue, INT64 lComparand) noexcept
		{
			return _InterlockedCompareExchange64(pnValue, nValue, lComparand);
		}

		// Implement other native types as translations to above.
#define INTERLOCK_REMAP(T,TI) \
	template<> inline T Increment<T>( T VOLATILE* pnValue ) noexcept \
	{ return (T) Increment<TI>( (TI VOLATILE*) pnValue); } \
	template<> inline T Decrement<T>( T VOLATILE* pnValue ) noexcept \
	{ return (T) Decrement<TI>( (TI VOLATILE*) pnValue); } \
	template<> inline T ExchangeAdd<T>( T VOLATILE* pnValue, T nValue  ) noexcept \
	{ return (T) ExchangeAdd<TI>( (TI VOLATILE*) pnValue, (TI) nValue ); } \
	template<> inline T Exchange<T>( T VOLATILE* pnValue, T nValue ) noexcept \
	{ return (T) Exchange<TI>( (TI VOLATILE*) pnValue,(TI)nValue); } \
	template<> inline T CompareExchange<T>( T VOLATILE* pnValue, T nValue, T lComparand ) noexcept \
	{ return (T) CompareExchange<TI>( (TI VOLATILE*) pnValue, (TI)nValue, (TI)lComparand); }

		INTERLOCK_REMAP(UINT, INTER32_t);
		INTERLOCK_REMAP(UINT64, INT64);

		// Special fix ups for the use of long vs int.

#ifdef _WIN32
		INTERLOCK_REMAP(int, INTER32_t);	// _WIN32 always uses LONG for 32 bits.
		INTERLOCK_REMAP(ULONG, INTER32_t);
#else
#if ! defined(USE_INT64)
		INTERLOCK_REMAP(long, INTER32_t);	// GNU 32 bit.
		INTERLOCK_REMAP(ULONG, INTER32_t);
#else
		// GNU 64 bit long is 64 bit. has no other _int64 intrinsic type.
#endif
#endif
	};

	//*************************************************************

	template< typename TYPE = INTER32_t >	// INTER32_t = 32 bit, INT64 for 64 bit.
	class GRAYCORE_LINK cInterlockedVal
	{
		//! @class Gray::cInterlockedVal
		//! thread interlocked/safe int/long. Not needed if ! _MT
		//! thread safe unitary actions on a INT32 value (>=32 bits)
		//! @note This uses inline __asm code and is fast!
		//! @note should use __DECL_ALIGN(X) to make sure we are aligned.

		typedef cInterlockedVal<TYPE> THIS_t;

	protected:
		TYPE VOLATILE m_nValue;		//!< This MUST be sizeof(TYPE) aligned?! __DECL_ALIGN(sizeof(TYPE)).

	public:
		cInterlockedVal(TYPE nValue = 0) noexcept
			: m_nValue(nValue)
		{
			ASSERT((((UINT_PTR)&m_nValue) % sizeof(TYPE)) == 0);	// must be aligned!!
		}

		TYPE Inc() noexcept	//! @return post increment. e.g. NEVER 0
		{
			return InterlockedN::Increment(&m_nValue);
		}
		void IncV() noexcept
		{
			InterlockedN::Increment(&m_nValue);
		}
		TYPE Dec() noexcept	//! @return post decrement.
		{
			return InterlockedN::Decrement(&m_nValue);
		}
		void DecV() noexcept
		{
			InterlockedN::Decrement(&m_nValue);
		}

		TYPE AddX(TYPE nValue) noexcept
		{
			//! @return pre-add value.
			return InterlockedN::ExchangeAdd(&m_nValue, nValue);
		}
		TYPE Exchange(TYPE nValue) noexcept
		{
			return InterlockedN::Exchange(&m_nValue, nValue);
		}

		TYPE CompareExchange(TYPE nValue, TYPE lComparand = 0) noexcept
		{
			//! only if current m_nValue is lComparand set the new m_nValue to nValue
			//! @return previous value.
			return InterlockedN::CompareExchange(&m_nValue, nValue, lComparand);
		}
		bool SetIfEqual(TYPE nValue, TYPE lComparand = 0) noexcept
		{
			return(lComparand == CompareExchange(nValue, lComparand));
		}

		inline TYPE operator ++() noexcept
		{
			//! @return The value post increment. e.g. NEVER 0
			return Inc();
		}
		inline TYPE operator --() noexcept
		{
			//! @return The value post decrement
			return Dec();
		}
		TYPE get_Value() const noexcept
		{
			return m_nValue;
		}
		void put_Value(TYPE nVal) noexcept
		{
			m_nValue = nVal;
		}
		operator TYPE () const noexcept
		{
			return m_nValue;
		}
		const THIS_t& operator = (TYPE nValNew) noexcept
		{
			Exchange(nValNew);
			return *this;
		}
	};

	// Base types=INT32, INT64. Derived types=UINT32,UINT64

	typedef __DECL_ALIGN(4) cInterlockedVal<INT32> cInterlockedInt32;
	typedef __DECL_ALIGN(4) cInterlockedVal<UINT32> cInterlockedUInt32;

	typedef __DECL_ALIGN(8) cInterlockedVal<INT64> cInterlockedInt64;
	typedef __DECL_ALIGN(8) cInterlockedVal<UINT64> cInterlockedUInt64;

	typedef __DECL_ALIGN(_SIZEOF_INT) cInterlockedVal<int> cInterlockedInt;	// default int type. whatever that is.
	typedef __DECL_ALIGN(_SIZEOF_INT) cInterlockedVal<UINT> cInterlockedUInt;

	typedef __DECL_ALIGN(_SIZEOF_LONG) cInterlockedVal<long> cInterlockedLong;
	typedef __DECL_ALIGN(_SIZEOF_LONG) cInterlockedVal<ULONG> cInterlockedULong;

	typedef __DECL_ALIGN(_SIZEOF_PTR) cInterlockedVal<INT_PTR> cInterlockedIntPtr;	// int that can also hold a pointer.

	//*****************************************************

	template< typename TYPE = void >	// INT_PTR = INT32 for 32 bit, INT64 for 64 bit.
	class GRAYCORE_LINK __DECL_ALIGN(_SIZEOF_PTR) cInterlockedPtr : protected cInterlockedVal < INT_PTR >
	{
		//! @class Gray::cInterlockedPtr
		//! An interlocked pointer to something. (pointer size may vary based on architecture)
		//! Cast it as needed.

	public:
		cInterlockedPtr(TYPE* pVal) noexcept : cInterlockedVal<INT_PTR>((INT_PTR)pVal)
		{
		}
		operator TYPE* () noexcept
		{
			return (TYPE*)(this->m_nValue);
		}
		operator const TYPE* () const noexcept
		{
			return (const TYPE*)(this->m_nValue);
		}

		const cInterlockedPtr<TYPE>& operator = (TYPE* pValNew)
		{
			// InterlockedExchangePointer()
			this->Exchange((INT_PTR)pValNew);
			return *this;
		}
	};

	typedef cInterlockedPtr<> cInterlockedPtrV;			// void pointer.

	//*****************************************************

	class GRAYCORE_LINK cInterlockedInc
	{
		//! @class Gray::cInterlockedInc
		//! Used as a thread safe check for code reentrancy. even on the same thread. like cLockableBase.
		//! define an instance of this on the stack. ALWAYS STACK BASED

	private:
		cInterlockedInt& m_rCount;	//!< pointer to the 'static' count.
		int m_nCount;	//!< the thread stable value of the count (post increment)
	public:
		cInterlockedInc(cInterlockedInt& count) noexcept
			: m_rCount(count)
			, m_nCount(count.Inc())
		{
		}
		~cInterlockedInc() noexcept
		{
			m_rCount.Dec();
		}

		int get_Count() const noexcept
		{
			return m_nCount;	//!< get the count as it was when we created this.
		}

		UNITTEST_FRIEND(cInterlockedVal);
	};

#ifndef GRAY_STATICLIB // force implementation/instantiate for DLL/SO.
	template class GRAYCORE_LINK cInterlockedPtr < >;
	template class GRAYCORE_LINK cInterlockedVal < long >;
	template class GRAYCORE_LINK cInterlockedVal < ULONG >;
#endif

}

#endif // _INC_cInterlockedVal_H
