//
//! @file cRandom.h
//! Basic random number generator.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cRandom_H
#define _INC_cRandom_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cSingleton.h"
#include "IUnknown.h"

namespace Gray
{
	DECLARE_INTERFACE(IRandomNoise)
	{
		//! @interface Gray::IRandomNoise
		//! Basic interface for getting noise (random data) from some entropy source.
		IGNORE_WARN_INTERFACE(IRandomNoise);
		virtual HRESULT GetNoise(void* pData, size_t iSize) = 0;	//!< fill array with random bytes. return # bytes filled.
	};

	class GRAYCORE_LINK cRandomBase : public IRandomNoise
	{
		//! @class Gray::cRandomBase
		//! Generic abstract base class for a integer/binary (pseudo) random number generator.
		//! Derived Providers will natively give up N bits of randomness per call/tick.
		//! Similar to .NET System.Random
		//! @note derived class MUST implement get_RandUns or GetRandUX to generate at least 32 bits or 31 bits of random data.

	public:
		typedef UINT SEED_t;		//!< default seed size might be 32 or 64 bit depending on k_RAND_MAX.

		cRandomBase() noexcept
		{
		}
		virtual ~cRandomBase()
		{
		}

		virtual void InitSeed(const void* pData, size_t iSize) = 0;	// all implementations must support this.
		void InitSeed(IRandomNoise* pSrc, size_t iSize);
		void InitSeedOS(size_t iSize = sizeof(int));
		void InitSeedUns(UINT iSeed);	// SEED_t

		virtual HRESULT GetNoise(void* pData, size_t iSize) override;	//!< fill array with random. return # filled.

		// Type helpers.
		virtual UINT get_RandUns();		// UINT_MAX // SEED_t
		bool GetRandBool()
		{
			//! flip a coin.
			return GetRandUX(2) == 1 ;
		}

		//! Get random number in scale. 0 to scale.
		//! unsigned integer is NON inclusive range. from 0 to nScale-1
		virtual UINT GetRandUX(UINT nScale); // get integer random number in desired interval. (Non inclusive)
		int GetRandIRange(int iRangeLo, int iRangeHi);    // output random int
	};

	class GRAYCORE_LINK cRandomPerf : public IRandomNoise, public cSingleton < cRandomPerf >
	{
		//! @class GraySSL::cRandomPerf
		//! prefer cRandomOS but use this as fallback

	public:
		cRandomPerf();
		static void GRAYCALL GetNoisePerf(void* pData, size_t iSize);
		virtual HRESULT GetNoise(void* pData, size_t iSize) override	// fill array with random. return # filled.
		{
			GetNoisePerf(pData, iSize);
			return (HRESULT)iSize;
		}
	};

	class GRAYCORE_LINK cRandomOS : public cRandomBase, public cSingleton < cRandomOS >
	{
		//! @class Gray::cRandomOS
		//! Get Low level Hardware based noise supplied by the OS. NO SEED. NOT Deterministic (in theory)
		//! __linux__ use "/dev/urandom" as a get_RandomSeed().
	private:
		virtual void InitSeed(const void* pData, size_t iSize)
		{
			// No way to seed this.
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(iSize);
		}

	public:
		cRandomOS();
		virtual ~cRandomOS();

		static HRESULT GRAYCALL GetNoiseOS(void* pData, size_t iSize);

		virtual HRESULT GetNoise(void* pData, size_t iSize) override;	// fill array with random. return # filled.
		virtual UINT get_RandUns();		// UINT_MAX

		CHEAPOBJECT_IMPL;
	};

	class GRAYCORE_LINK cRandomBlock : public IRandomNoise
	{
		//! @class Gray::cRandomBlock
		//! Hold a blob of random data. Acts as a one time cipher.
		//! Supply test 'random' data. (e.g. maybe not random at all)

	public:
		cMemBlock m_Src;		// a block of 'random' test data. 
		size_t m_nOffset;		// How far have we read in m_Src? recycle when at end ?

	public:
		cRandomBlock(const void* pData, size_t nSize) noexcept
			: m_Src(pData, nSize)
			, m_nOffset(0)
		{
		}
		virtual HRESULT GetNoise(void* pData, size_t len) override	// IRandomNoise
		{
			//! Get sample random data bytes
			if (m_Src.isValidPtr())
			{
				// todo repeat like cMem::CopyRepeat() ?
				cMem::Copy(pData, m_Src.GetSpan(m_nOffset, len), len);
			}
			else
			{
				// No m_Src supplied so fill with fixed data.
				cMem::Fill(pData, len, 0x2a);		
			}
			m_nOffset += len;
			return (HRESULT)len;
		}
	};

	class GRAYCORE_LINK cRandomDef : public cRandomBase
	{
		//! @class Gray::cRandomDef
		//! Like the default 'C' library seeded pseudo-random number generator ::srand() and ::rand()
		//! Control a series of pseudo random numbers via a seed. 
		//! not thread safe. Use cThreadLocal to make thread safe.
		//! RESOLUTION = k_RAND_MAX

	public:
		static const SEED_t k_RAND_MAX = 0x7fff;	//!< 0x7fff = RAND_MAX, 31 bits of random.

	private:
		SEED_t m_nSeed;	//!< Control the pattern of random numbers via the seed. may be globally/thread shared.

	public:
		cRandomDef(SEED_t nSeed = 1);
		virtual ~cRandomDef();

		SEED_t GetRandNext();

		virtual void InitSeed(const void* pData, size_t iSize) override;	// Start a repeatable seeded series
		virtual UINT GetRandUX(UINT nScale) override; // k_RAND_MAX is not always the same as UINT.
	};

	extern GRAYCORE_LINK cRandomDef g_Rand;	//!< the global random number generator. NOT thread safe. but does that matter?
} 
#endif
