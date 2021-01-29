//
//! @file cRandom.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cRandom.h"
#include "cHeap.h"

#ifdef __linux__
#include <fcntl.h>	// open()
#endif

namespace Gray
{
	GRAYCORE_LINK cRandomDef g_Rand;	//! the default random number generator. NOT Thread Safe!

#if defined(_WIN32) && ! defined(_MSC_VER)
	// extern for CryptAcquireContext
#endif

	//*************************************************************

	void cRandomBase::InitSeed(IRandomNoise* pSrc, size_t iSize)
	{
		cHeapBlock block(iSize);
		HRESULT iSizeRet = pSrc->GetNoise(block.get_Data(), iSize);
		InitSeed(block.get_Data(), (size_t)iSizeRet);	// pure virtual
	}

	void cRandomBase::InitSeedDefault(size_t iSize)
	{
		//! Initialize random sequence randomly.
		//! Seed the random generator with time or some mix of more random data. NOT Deterministic!
		InitSeed(cRandomOS::get_Single(), iSize);
	}

	void cRandomBase::InitSeedUns(UINT nSeed)
	{
		//! set seed from UINT. Similar to SEED_t
		InitSeed(&nSeed, sizeof(nSeed));
	}

	UINT cRandomBase::get_RandUns() // virtual
	{
		//! get UINT random number in 0 to < UINT_MAX range
		//! default implementation.
		//! @note derived implementation MUST override get_RandUns or GetRandUX
		return GetRandUX(UINT_MAX);
	}

	UINT cRandomBase::GetRandUX(UINT nScale) // virtual
	{
		//! get random integer number in 0 to < nScale range
		//! NON inclusive
		//! default implementation.
		//! @note derived implementation MUST override get_RandUns() or GetRandUX()
		if (nScale <= 1)
			return 0;  // error
		UINT uVal = (get_RandUns() % nScale);	// shave off the part i don't want.
		return uVal;
	}

	int cRandomBase::GetRandIRange(int iRangeLo, int iRangeHi)    // output random int
	{
		//! get a random integer inclusive inside range. default implementation.
		//! @return a random number in an inclusive range of integers.

		int iRange = (iRangeHi - iRangeLo) + 1;
		if (iRange == 0)
			return iRangeLo;
		if (iRange < 0)
			return iRangeLo - GetRandUX(-iRange);
		else
			return iRangeLo + GetRandUX(iRange);
	}

	HRESULT cRandomBase::GetNoise(void* pvData, size_t iSize) // override
	{
		//! fill a buffer with random bytes of data.
		//! < 0 = error.
		UINT* puData = (UINT*)pvData;
		size_t iSizeLeft = iSize;
		while (iSizeLeft > 0)
		{
			UINT uVal = get_RandUns();	// virtual.
			if (iSizeLeft < sizeof(uVal))
			{
				// running out of room.
				::memcpy(puData, &uVal, iSizeLeft);	// partial fill.
				break;	// done.
			}
			puData[0] = uVal;
			puData++;
			iSizeLeft -= sizeof(uVal);
		}
		return (HRESULT)iSize;
	}

	//*************************************************************
	cRandomPerf::cRandomPerf()
		: cSingleton<cRandomPerf>(this, typeid(cRandomPerf))
	{
	}
	void GRAYCALL cRandomPerf::GetNoisePerf(void* pData, size_t iSize) // static
	{
		//! Get noise via low bits of cTimePerf
		//! Prefer GetNoiseOS noise over this, but use this as a fallback or XOR.

		UINT* puData = (UINT*)pData;
		size_t iSizeLeft = iSize;
		while (iSizeLeft > 0)
		{
			cTimePerf tStart(true);		// the low bits of high performance timer should be random-ish.
			UINT32 uVal = (UINT32)(tStart.m_nTime ^ 3141592654UL);	// use PI as default XOR pattern
			if (iSizeLeft < sizeof(uVal))
			{
				// running out of room.
				::memcpy(puData, &uVal, iSizeLeft);
				break;	// done.
			}
			puData[0] = uVal;
			puData++;
			iSizeLeft -= sizeof(uVal);
		}
	}

	//*************************************************************

	cRandomOS::cRandomOS()
		: cSingleton<cRandomOS>(this, typeid(cRandomOS))
	{
	}
	cRandomOS::~cRandomOS()
	{
	}

	HRESULT GRAYCALL cRandomOS::GetNoiseOS(void* pData, size_t iSize)	// static. fill array with random. return # filled.
	{
		//! Try to use the OS supplied noise generator. It may not work.
#ifdef __linux__
	// Is the Linux version high enough for random function ?

#if defined(SYS_getrandom)
		if (cSystemInfo::I().isVer3_17_plus())
		{
			// long sys_getrandom(char __user *buf, size_t count, unsigned int flags)
			int iRet = ::syscall(SYS_getrandom, pData, iSize, 0);
			if (iRet > 0)
			{
				return iRet;
			}
		}
#endif

		cOSHandle fd;
		fd.OpenHandle("/dev/random", O_RDONLY);
		if (!fd.isValidHandle())
			fd.OpenHandle("/dev/urandom", O_RDONLY);
		if (fd.isValidHandle())
		{
			int iSizeRet = fd.ReadX(pData, iSize);
			if (iSizeRet != (int)iSize)
				return E_FAIL;
			return iSize;
		}

#elif defined(_WIN32) && defined(_MSC_VER) && defined(WINCRYPT32API)
		// WinCrypt
		HCRYPTPROV hProvider;	// NOT a pointer so its not a type-ablefor cOSHandle or cHandlePtr
		if (_FNF(::CryptAcquireContext)(&hProvider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		{
			bool bRet = ::CryptGenRandom(hProvider, (DWORD)iSize, (BYTE*)pData);
			::CryptReleaseContext(hProvider, 0);
			if (bRet)
			{
				return (HRESULT)iSize;
			}
		}
#endif
		return E_NOTIMPL;	// OS noise not available. Try some other pseudo random source.
	}


	HRESULT cRandomOS::GetNoise(void* pData, size_t iSize)	// override. fill array with random. return # filled.
	{
		//! Get the best source or random noise the system can supply.
		//! Make a random seed from the most random source we can find.
		if (iSize <= 0)
			return  (HRESULT)iSize;
		HRESULT hRes = GetNoiseOS(pData, iSize);
		if (hRes >= (HRESULT)iSize)
			return hRes;
		// else fallback to try the high perf timer.
		cRandomPerf::GetNoisePerf(pData, iSize);
		return (HRESULT)iSize;
	}

	UINT cRandomOS::get_RandUns() // virtual
	{
		// UINT_MAX
		UINT uVal = 0;
		GetNoise(&uVal, sizeof(uVal));
		return uVal;
	}

	//*************************************************************

#if 0
	template<typename TYPE>
	TYPE rand_range(TYPE min_, TYPE max_)
	{
		//! @todo implement a template<> for getting random of correct type ??
		return static_cast<TYPE>(((static_cast<float>(g_Rand.GetRandNext()) / static_cast<float>(k_RAND_MAX))) * (max_ - min_) + min_);
	}
#endif

	cRandomDef::cRandomDef(SEED_t nSeed)
		: m_nSeed(nSeed)
	{
		//! like ::srand()
		//! http://stackoverflow.com/questions/4768180/rand-implementation
	}
	cRandomDef::~cRandomDef()
	{
	}

	void cRandomDef::InitSeed(const void* pData, size_t iSize)	// override
	{
		//! Start a repeatable series of pseudo random numbers
		//! like ::srand()
		if (iSize > sizeof(m_nSeed))
			iSize = sizeof(m_nSeed);
		m_nSeed = 0;	// Clear any under flow.
		::memcpy(&m_nSeed, pData, iSize);
	}

	UINT cRandomDef::GetRandNext()
	{
		//! Get next pseudo random number like ::rand();
		//! k_RAND_MAX assumed to be 32767
#ifdef __linux__
		m_nSeed = m_nSeed * 1103515245 + 12345;
#else 
		m_nSeed = m_nSeed * 214013L + 2531011L;
#endif 
		return (m_nSeed >> 16) & k_RAND_MAX;
	}

	UINT cRandomDef::GetRandUX(UINT nScale) // override
	{
		//! @return value from 0 to less than nScale. (non inclusive of nScale)
		//! granularity may be effected by k_RAND_MAX (much larger for Linux) like rand()
		//! @note: _WIN32 rand() is NOT thread safe. 2 threads will get the same value if called at the same time !!

		if (nScale <= 1)
			return 0;

		UINT uRand = GetRandNext();
		if (nScale == k_RAND_MAX)
		{
			return uRand;
		}

		// Make additional calls to ::rand() to get more random bits.
		if (nScale > k_RAND_MAX)
		{
			UINT uScaleCur = nScale;
			do
			{
				uRand *= k_RAND_MAX;
				uRand += GetRandNext();
				uScaleCur /= k_RAND_MAX;
			} while (uScaleCur > k_RAND_MAX);
		}

		return uRand % nScale ;	// chop off extra junk high bits.
	}
}
