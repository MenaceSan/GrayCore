//
//! @file CRandomDef.h
//! Basic random number generator.
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CRandomDef_H
#define _INC_CRandomDef_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CSingleton.h"
#include "IUnknown.h"

UNITTEST_PREDEF(CRandomBase)

namespace Gray
{
	DECLARE_INTERFACE(IRandomNoise)
	{
		//! @interface Gray::IRandomNoise
		//! Basic interface for getting noise (random data) from some entropy source.
		IGNORE_WARN_INTERFACE(IRandomNoise);
		virtual HRESULT GetNoise(void* pData, size_t iSize) = 0;	//!< fill array with random bytes. return # bytes filled.
	};

	class GRAYCORE_LINK CRandomBase : public IRandomNoise
	{
		//! @class Gray::CRandomBase
		//! Generic abstract base class for a integer/binary (pseudo) random number generator.
		//! Similar to .NET System.Random
		//! @note derived class MUST implement get_RandUns or GetRandUX

	public:
		typedef UINT SEED_t;		//!< default seed size might be 32 or 64 bit depending on k_RAND_MAX.

		CRandomBase()
		{
		}
		virtual ~CRandomBase()
		{
		}

		virtual void InitSeed(const void* pData, size_t iSize) = 0;	// all implementations must support this.
		void InitSeed(IRandomNoise* pSrc, size_t iSize);
		void InitSeedDefault(size_t iSize = sizeof(int));
		void InitSeedUns(UINT iSeed);	// SEED_t

		virtual HRESULT GetNoise(void* pData, size_t iSize) override;	//!< fill array with random. return # filled.

		// Type helpers.
		virtual UINT get_RandUns();		// UINT_MAX // SEED_t
		bool GetRandBool()
		{
			//! flip a coin.
			return(GetRandUX(2) == 1);
		}

		//! Get random number in scale. 0 to scale.
		//! unsigned integer is NON inclusive range. from 0 to nScale-1
		virtual UINT GetRandUX(UINT nScale); // get integer random number in desired interval. (Non inclusive)
		int GetRandIRange(int iRangeLo, int iRangeHi);    // output random int

		UNITTEST_FRIEND(CRandomBase);
	};

	class GRAYCORE_LINK CRandomNoise : public CRandomBase, public CSingleton < CRandomNoise >
	{
		//! @class Gray::CRandomNoise
		//! Get Low level Hardware based noise supplied by the OS.
		//! __linux__ use "/dev/urandom" as a get_RandomSeed().
	private:
		virtual void InitSeed(const void* pData, size_t iSize)
		{
			// No way to seed this.
			UNREFERENCED_PARAMETER(pData);
			UNREFERENCED_PARAMETER(iSize);
		}

	public:
		CRandomNoise();
		virtual ~CRandomNoise();

		static HRESULT GRAYCALL GetNoiseOS(void* pData, size_t iSize);
		static void GRAYCALL GetNoisePerf(void* pData, size_t iSize);

		virtual HRESULT GetNoise(void* pData, size_t iSize) override;	// fill array with random. return # filled.
		virtual UINT get_RandUns();		// UINT_MAX

		CHEAPOBJECT_IMPL;
	};

	class GRAYCORE_LINK CRandomDef : public CRandomBase
	{
		//! @class Gray::CRandomDef
		//! Like the default 'C' library seeded pseudo-random number generator srand() and rand()
		//! Control a series of pseudo random numbers via a seed. 
		//! not thread safe. Use CThreadLocal to make thread safe.
		//! RESOLUTION = k_RAND_MAX

	public:
		static const SEED_t k_RAND_MAX = 0x7fff;	//!< 0x7fff = RAND_MAX

	private:
		SEED_t m_nSeed;	//!< Control the pattern of random numbers via the seed. may be globally/thread shared.

	public:
		CRandomDef(SEED_t nSeed = 1);
		virtual ~CRandomDef();

		SEED_t GetRandNext();

		virtual void InitSeed(const void* pData, size_t iSize) override;	// Start a repeatable seeded series
		virtual UINT GetRandUX(UINT nScale) override; // k_RAND_MAX is not the same as UINT.
	};

#ifdef USE_UNITTESTS
	class cRandomUnitTest : public IRandomNoise
	{
		//! @class Gray::cRandomUnitTest
		//! Supply test 'random' data. (e.g. not random at all)

	public:
		CMemBlock m_Src;		// a block of 'random' test data. 
		size_t m_nOffset;		// How far have we read?

	public:
		cRandomUnitTest(const void* pData, size_t nSize)
			: m_Src(nSize, pData)
			, m_nOffset(0)
		{
		}
		virtual HRESULT GetNoise(void* pData, size_t len) override	// IRandomNoise
		{
			//! Get sample random data bytes
			if (m_Src.get_Start() == nullptr)
			{
				// No m_Src supplied so fill with fixed data.
				CMem::Fill(pData, len, 0x2a);
				m_nOffset += len;
			}
			else
			{
				// todo repeat like CMem::CopyRepeat() ?
				::memcpy(pData, m_Src.GetOffset(m_nOffset), len);
				m_nOffset += len;
				ASSERT(m_Src.IsValidIndex2(m_nOffset));		// Don't overflow!
			}
			return (HRESULT)len;
		}
	};
#endif

	extern GRAYCORE_LINK CRandomDef g_Rand;	//!< the global random number generator. NOT thread safe. but does that matter?
};
#endif
