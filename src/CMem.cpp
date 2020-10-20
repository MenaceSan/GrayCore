//
//! @file CMem.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "CMem.h"
#include "StrT.h"
#include "HResult.h"
#include "CDebugAssert.h"
#include "CExceptionSystem.h"

namespace Gray
{

#if ! defined(UNDER_CE) && ! defined(_CPPUNWIND)
#include <setjmp.h>
#include <signal.h>

	static jmp_buf s_CMem_IsValidFailJmpBuf;
	typedef void(__cdecl * JMP_t)(int);

	void __cdecl CMem::IsValidFailHandler(int nSig)	// JMP_t static
	{
		//! @todo make s_CMem_IsValidFailJmpBuf thread safe.
		UNREFERENCED_PARAMETER(nSig);
		::longjmp(s_CMem_IsValidFailJmpBuf, 1);
	}
#endif

	uintptr_t VOLATILE CMem::sm_bDontOptimizeOut0 = 0;	//!< used to trick the optimizer. Always 0.
	uintptr_t VOLATILE CMem::sm_bDontOptimizeOutX = 1;	//!< used to trick the optimizer. Unknown value.

	bool GRAYCALL CMem::IsValid(const void* pData, size_t nLen, bool bWriteAccess) // static
	{
		//! Is this pointer valid to read/write to ? On heap, stack or static const data space.
		//! similar to _MFC_VER AfxIsValidAddress(), AtlIsValidAddress() 
		//! @note this should only ever be used in _DEBUG code. and only in an ASSERT. Its slow.
		//! IsBadWritePtr() is just doing exception handling under the hood. _CrtIsValidPointer is just a null check in >= VS2010 
		//! @note This can cause problems with thread stack guard pages in theory. https://msdn.microsoft.com/en-us/library/bb288454.aspx
		//! @todo make a faster (less thorough) version of this ?

		if (pData == nullptr)	// nullptr is never valid.
		{
			if (nLen == 0)
				return true;	// This technically is valid.
			return false;
		}
		if (!IsValidApp(pData))	// ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space. <1G
		{
			return false;
		}

#if defined(UNDER_CE)
		if (bWriteAccess)
		{
			return ::IsBadWritePtr(const_cast<void*>(pData), nLen);
		}
		return ::IsBadReadPtr(pData, nLen);
#elif defined(_CPPUNWIND)
		// They say IsBadReadPtr() isn't as good as exceptions. So don't use it.
		UNREFERENCED_PARAMETER(bWriteAccess);
		
		GRAY_TRY // GRAY_TRY // try
		{
			// NOTE: Aligned blocks might make this faster.
			sm_bDontOptimizeOut0 = 0;	// Always 0
			for (size_t i = 0; i < nLen; i++)
			{
				BYTE bVal = ((const BYTE*)pData)[i];	// Read it.
				if (bWriteAccess)	// try to write it.
				{
					((BYTE*)pData)[i] = bVal | (BYTE)sm_bDontOptimizeOut0;
				}
			}
			return true;
		}
		GRAY_TRY_CATCHALL // GRAY_TRY_CATCHALL // catch(...)
		{
			// We failed.
			return false;
		}
		GRAY_TRY_END
#else
		//! __linux__ / POSIX version of IsBadReadPtr()
		UNREFERENCED_PARAMETER(bWriteAccess);  // TODO bWriteAccess
		JMP_t pfnPrevHandler = nullptr;
		if (::setjmp(s_CMem_IsValidFailJmpBuf))
		{
			// We failed.
			::signal(SIGSEGV, pfnPrevHandler);
			return false;
		}
		pfnPrevHandler = ::signal(SIGSEGV, IsValidFailHandler);

		// NOTE: Aligned blocks might make this faster.
		sm_bDontOptimizeOut0 = 0;	// Always 0
		for (size_t i = 0; i < nLen; i++)
		{
			BYTE bVal = ((const BYTE*)pData)[i];	// Read it.
			if (bWriteAccess)	// try to write it.
			{
				((BYTE*)pData)[i] = bVal | (BYTE)sm_bDontOptimizeOut0;
			}
		}

		::signal(SIGSEGV, pfnPrevHandler);		// undo signal.
		return true;
#endif
	}

	size_t GRAYCALL CMem::CompareIndex(const void* p1, const void* p2, size_t nSizeBlock) // static 
	{
		//! Compare two buffers and return at what point they differ.
		//! Does not assume memory alignment for uintptr_t block compares.

		if (p1 == p2)
			return nSizeBlock;
		if (p1 == nullptr || p2 == nullptr)
			return 0;

		size_t i = 0;

		if (nSizeBlock >= sizeof(uintptr_t))
		{
			// TODO Alignment of pointers may be important?!? if not uintptr_t aligned this will fail on PPC?

			uintptr_t nDiffA = 0;
			size_t nSizeBlockA = nSizeBlock;
			if ((nSizeBlock & (sizeof(uintptr_t) - 1)) != 0)
			{
				nSizeBlockA &= ~(sizeof(uintptr_t) - 1); // trim any unaligned part off the end for this loop.
			}

			for (; i < nSizeBlockA; i += sizeof(uintptr_t))
			{
				nDiffA |= ((const uintptr_t*)p1)[i] ^ ((const uintptr_t*)p2)[i];
				if (nDiffA != 0)	// NOT the same. 
					break;		// now find the exact byte in the block.
			}
		}

		// Do odd part at the end.
		BYTE nDiffB = 0;
		for (; i < nSizeBlock; i++)
		{
			nDiffB |= ((const BYTE*)p1)[i] ^ ((const BYTE*)p2)[i];
			if (nDiffB != 0)	// NOT the same.
				return i;
		}

		return nSizeBlock;
	}

	StrLen_t GRAYCALL CMem::ConvertToString(char* pszDst, StrLen_t iSizeDstMax, const BYTE* pSrc, size_t nSrcQty) // static
	{
		//! Write bytes out to a string as comma separated base 10 number values. 
		//! Try to use SetHexDigest() instead.
		//! opposite of CMem::ReadFromString().
		//! @return the actual size of the string.
		//! @note using Base64 would be better.

		iSizeDstMax -= 4;	// room to terminate < max sized number.
		StrLen_t iLenOut = 0;
		for (size_t i = 0; i < nSrcQty; i++)
		{
			if (i > 0)
			{
				pszDst[iLenOut++] = ',';
			}

			StrLen_t iLenThis = StrNum::UtoA(pSrc[i], pszDst + iLenOut, iSizeDstMax - iLenOut, 10);
			if (iLenThis <= 0)
				break;
			iLenOut += iLenThis;
			if (iLenOut >= iSizeDstMax)
				break;
		}
		return iLenOut;
	}

	size_t GRAYCALL CMem::ReadFromString(BYTE* pOut, size_t iDstMax, const char* pszSrc)
	{
		//! Read/Parse bytes in from string as comma separated base 10 number values. opposite of CMem::ConvertToString().
		//! @return the number of bytes read.
		//! @note using Base64 would be better.

		size_t i = 0;
		for (; i < iDstMax;)
		{
			pszSrc = StrT::GetNonWhitespace(pszSrc);
			if (StrT::IsNullOrEmpty(pszSrc))
				break;
			const char* pszSrcStart = pszSrc;
			pOut[i++] = (BYTE)(StrNum::toU(pszSrc, &pszSrc, 10));
			if (pszSrcStart == pszSrc)	// must be the field terminator? ")},;". End.
				break;
			pszSrc = StrT::GetNonWhitespace(pszSrc);
			if (pszSrc[0] != ',')
				break;
			pszSrc++;
		}
		return i;
	}

	StrLen_t GRAYCALL CMem::GetHexDigest(OUT char* pszHexString, const BYTE* pData, size_t nSizeData) // static 
	{
		//! Get the final hash as a pre-formatted string of hex digits. opposite of CMem::SetHexDigest
		//! ASSUME sizeof(pszHexString) >= SizeHexDigest
		//! @note using Base64 would be better.
		StrLen_t iLen = 0;
		for (size_t i = 0; i < nSizeData; i++)
		{
			BYTE ch = pData[i];
			pszHexString[iLen++] = StrChar::U2Hex(ch >> 4);
			pszHexString[iLen++] = StrChar::U2Hex(ch & 0x0F);
		}
		pszHexString[iLen] = '\0';
		return iLen;
	}

	HRESULT GRAYCALL CMem::SetHexDigest(const char* pszHexString, OUT BYTE* pData, size_t nSizeData) // static 
	{
		//! Set binary pDigest from string pszHexString
		//! opposite of CMem::GetHexDigest
		//! @arg nSizeData = The number of expected output digits.
		//! @note using Base64 would be better.
		//! @return S_FALSE = was zero value.

		if (pszHexString == nullptr)
			return E_POINTER;
		bool bNonZero = false;
		for (size_t i = 0; i < nSizeData; i++)
		{
			// 2 hex chars per byte.
			if (*pszHexString == '\0') // not long enough!
				return HRESULT_WIN32_C(RPC_X_BYTE_COUNT_TOO_SMALL);
			int bHex1 = StrChar::Hex2U(*pszHexString++);
			if (bHex1 < 0)
				return HRESULT_WIN32_C(ERROR_SXS_XML_E_INVALID_HEXIDECIMAL);
			int bHex2 = StrChar::Hex2U(*pszHexString++);
			if (bHex2 < 0)
				return HRESULT_WIN32_C(ERROR_SXS_XML_E_INVALID_HEXIDECIMAL);
			pData[i] = (BYTE)((bHex1 * 0x10) + bHex2);
			if (pData[i] != 0)
				bNonZero = true;
		}
		if (StrChar::IsDigitX(*pszHexString, 0x10)) // too long! or not terminated.
			return HRESULT_WIN32_C(ERROR_BUFFER_OVERFLOW);
		if (bNonZero)
			return S_OK;
		return S_FALSE;	// zero
	}

	//**********************************************************

	COMPARE_t GRAYCALL CMemBlock::Compare(const void* pData1, size_t iLen1, const void* pData2, size_t iLen2) // static
	{
		//! @return COMPARE_Equal
		size_t iLenMin = MIN(iLen1, iLen2);
		COMPARE_t iRet = CMem::Compare(pData1, pData2, iLenMin);
		if (iRet != COMPARE_Equal)
			return iRet;
		return CValT::Compare(iLen1, iLen2);	// longer wins. if otherwise equal. (but not same length)
	}

	//**********************************************************

	void GRAYCALL CValArray::ReverseArrayBlocks(void* pArray, size_t nArraySizeBytes, size_t nBlockSize)
	{
		//! reverse the order of an array of blocks/objects.
		//! similar to ReverseArray() but without the TYPE. where nBlockSize == sizeof(TYPE)
		//! @arg
		//!  nArraySizeBytes = the size of the whole array in bytes.
		//!  nBlockSize = size of array element in bytes.

		ASSERT(nBlockSize > 0);
		size_t nBlockOverflow = nArraySizeBytes % nBlockSize;
		ASSERT(nBlockOverflow == 0); // MUST be aligned!
		UNREFERENCED_PARAMETER(nBlockOverflow);
		// nArraySizeBytes -= nBlockOverflow;

		// Optimize the intrinsic sized blocks first.
		switch (nBlockSize)
		{
		case sizeof(BYTE) :
			ReverseArray<BYTE>((BYTE*)pArray, nArraySizeBytes);
			return;
		case sizeof(WORD) :
			ReverseArray<WORD>((WORD*)pArray, nArraySizeBytes);
			return;
		case sizeof(UINT32) :
			ReverseArray<UINT32>((UINT32*)pArray, nArraySizeBytes);
			return;
		}

		// array of arbitrary block size.

		BYTE* pMemBS = (BYTE*)pArray;
		BYTE* pMemBE = ((BYTE*)pArray) + nArraySizeBytes - nBlockSize; // end

		for (; pMemBS < pMemBE; pMemBS += nBlockSize, pMemBE -= nBlockSize)
		{
			CMem::Swap(pMemBS, pMemBE, nBlockSize);
		}
	}
}

//*************************************************************************
#if USE_UNITTESTS
#include "CUnitTest.h"
#include "CLogMgr.h"

UNITTEST_CLASS(CMem)
{
	template <class TYPE>
	void UnitTest2(const TYPE nValH)
	{
		TYPE nValRev = nValH;
		CValArray::ReverseArrayBlocks(&nValRev, sizeof(nValRev), 1);

		TYPE nValRev2 = nValH;
		CValArray::ReverseArray<BYTE>((BYTE*)&nValRev2, sizeof(nValRev2));
		UNITTEST_TRUE(nValRev2 == nValRev);

		TYPE nValRev3 = CMemT::ReverseType(nValH);
		UNITTEST_TRUE(nValRev3 == nValRev);

		TYPE nValN = CMemT::HtoN(nValH);
#ifdef USE_LITTLE_ENDIAN
		// Bytes must be reversed.
		UNITTEST_TRUE(nValN == nValRev);
#else
		UNITTEST_TRUE(nValN == nValH);	// no change
#endif
		UNITTEST_TRUE(CMemT::NtoH(nValN) == nValH);	// back to original value.
	}

	UNITTEST_METHOD(CMem)
	{
		// IsValid
		static const int k_Val = 123;	// i should not be able to write to this !?
		UNITTEST_TRUE(CMem::IsValid(&k_Val, 1, false));	// read static/const memory is valid.

		// Write to nullptr and low memory?
		UNITTEST_TRUE(!CMem::IsValid(nullptr, 1));
		UNITTEST_TRUE(!CMem::IsValid((void*)12, 1, true));

#if 0 // This doesn't work well in VS2015 in x64 for some reason. Does not continue from exception.
		// Write to const/ROM space?
		bool bCanWriteToROM = CMem::IsValid(&k_Val, 1, true);
		UNITTEST_TRUE(!bCanWriteToROM);	// writing to const should NOT be valid !?

		// Write to Code?
		bool bCanWriteToCode = CMem::IsValid((void*)&CMem::CompareIndex, 1, true);
		UNITTEST_TRUE(!bCanWriteToCode);
#endif

		// CompareIndex
		BYTE szTmp1[32];
		BYTE szTmp2[32];
		CMem::Fill(szTmp1, sizeof(szTmp1), 1);
		CMem::Fill(szTmp2, sizeof(szTmp2), 2);
		size_t nRet = CMem::CompareIndex(szTmp1, szTmp2, 4);
		UNITTEST_TRUE(nRet == 0);

		szTmp1[0] = 2;
		nRet = CMem::CompareIndex(szTmp1, szTmp2, 8);
		UNITTEST_TRUE(nRet == 1);

		nRet = CMem::CompareIndex(szTmp1, szTmp2, 9);
		UNITTEST_TRUE(nRet == 1);

		CMem::Copy(szTmp1, szTmp2, sizeof(szTmp2));
		nRet = CMem::CompareIndex(szTmp1, szTmp2, 9);
		UNITTEST_TRUE(nRet == 9);

		// Host order test data.
		UnitTest2<WORD>(0x1234);
		UnitTest2<UINT32>(0x12345678);
		UnitTest2<UINT64>(0x123456789abcdef0ULL);
		UnitTest2<ULONG>(0x12345678);	// Maybe 32 or 64 bit ?

		char szTmp[k_TEXTBLOB_LEN * 4 + 10];
		StrLen_t nLen = CMem::ConvertToString(szTmp, STRMAX(szTmp), (const BYTE*)(const char*)k_sTextBlob.m_A, k_TEXTBLOB_LEN);
		UNITTEST_TRUE(nLen >= k_TEXTBLOB_LEN);

		BYTE bTmp[k_TEXTBLOB_LEN + 10];
		size_t nSizeRet = CMem::ReadFromString(bTmp, STRMAX(bTmp), szTmp);
		UNITTEST_TRUE(nSizeRet == (size_t)k_TEXTBLOB_LEN);
		UNITTEST_TRUE(!CMem::Compare(bTmp, (const char*)k_sTextBlob.m_A, nSizeRet));
		UNITTEST_TRUE(CMem::IsValid(szTmp));
		UNITTEST_TRUE(CMem::IsValid(bTmp));

	}
};
UNITTEST_REGISTER(CMem, UNITTEST_LEVEL_Core);
#endif
