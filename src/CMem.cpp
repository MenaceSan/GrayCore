//
//! @file cMem.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//
#include "pch.h"
#include "cMem.h"
#include "StrT.h"
#include "HResult.h"
#include "cDebugAssert.h"
#include "cExceptionSystem.h"

namespace Gray
{

#if USE_CRT && ! defined(UNDER_CE) && ! defined(_CPPUNWIND)
#include <setjmp.h>
#include <signal.h>

	static jmp_buf s_CMem_IsValidFailJmpBuf;
	typedef void(__cdecl * JMP_t)(int);

	void __cdecl cMem::IsValidFailHandler(int nSig)	// JMP_t static
	{
		//! @todo make s_CMem_IsValidFailJmpBuf thread safe.
		UNREFERENCED_PARAMETER(nSig);
		::longjmp(s_CMem_IsValidFailJmpBuf, 1);
	}
#endif

	uintptr_t VOLATILE cMem::sm_bDontOptimizeOut0 = 0;	//!< used to trick the optimizer. Always 0.
	uintptr_t VOLATILE cMem::sm_bDontOptimizeOutX = 1;	//!< used to trick the optimizer. Unknown value.
 
	const cMemBlock cMemBlock::k_EmptyBlock ; // const

	bool GRAYCALL cMem::IsCorrupt(const void* pData, size_t nLen, bool bWriteAccess) noexcept // static
	{
		//! Is this pointer valid to read/write to ? On heap, stack or static const data space.
		//! similar to _MFC_VER AfxIsValidAddress(), AtlIsValidAddress() 
		//! @note This IS slow. This should only ever be used in _DEBUG code. and only in an ASSERT. 
		//! IsBadWritePtr() is just doing exception handling under the hood. _CrtIsValidPointer is just a null check in >= VS2010 
		//! @note This can cause problems with thread stack guard pages in theory. https://msdn.microsoft.com/en-us/library/bb288454.aspx
		//! @todo make a faster (less thorough) version of this ?

		if (pData == nullptr)	// nullptr is not technically bad unless nLen > 0.
		{
			if (nLen == 0)
				return false;	// This technically is ok.
			return true;
		}
		if (!IsValidApp(pData))	// ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space. <1G
		{
			return true;
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
			for (size_t i = 0; i < nLen; i += k_PageSizeMin)
			{
				BYTE bVal = ((const BYTE*)pData)[i];	// Read it.
				if (bWriteAccess)	// try to write it.
				{
					((BYTE*)pData)[i] = bVal | (BYTE)sm_bDontOptimizeOut0;
				}
			}
			return false;
		}
		GRAY_TRY_CATCHALL // GRAY_TRY_CATCHALL // catch(...)
		{
			return true;	// We failed.
		}
		GRAY_TRY_END
#elif USE_CRT
		//! __linux__ / POSIX version of IsBadReadPtr()
		UNREFERENCED_PARAMETER(bWriteAccess);  // TODO bWriteAccess
		JMP_t pfnPrevHandler = nullptr;
		if (::setjmp(s_CMem_IsValidFailJmpBuf))
		{
			// We failed.
			::signal(SIGSEGV, pfnPrevHandler);
			return true;
		}
		pfnPrevHandler = ::signal(SIGSEGV, IsValidFailHandler);

		// NOTE: Aligned blocks might make this faster.
		sm_bDontOptimizeOut0 = 0;	// Always 0
		for (size_t i = 0; i < nLen; i += k_PageSizeMin)
		{
			BYTE bVal = ((const BYTE*)pData)[i];	// try to Read it.
			if (bWriteAccess)	// try to write it.
			{
				((BYTE*)pData)[i] = bVal | (BYTE)sm_bDontOptimizeOut0;
			}
		}

		::signal(SIGSEGV, pfnPrevHandler);		// undo signal.
		return false;	// its good.
#else
		return false;
#endif
	}

	size_t GRAYCALL cMem::CompareIndex(const void* p1, const void* p2, size_t nSizeBlock) // static 
	{
		//! Compare two buffers and return at what point they differ.
		//! Does not assume memory alignment for uintptr_t block compares.

		if (p1 == p2)
			return nSizeBlock; // is equal.
		if (p1 == nullptr || p2 == nullptr)
			return 0;

		size_t i = 0;

		if (nSizeBlock >= sizeof(uintptr_t))	// compare using max size registers.
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

		// Do odd part at the end. BYTE aligned.
		BYTE nDiffB = 0;
		for (; i < nSizeBlock; i++)
		{
			nDiffB |= ((const BYTE*)p1)[i] ^ ((const BYTE*)p2)[i];
			if (nDiffB != 0)	// NOT the same.
				return i;
		}

		return nSizeBlock;	// is equal.
	}

	StrLen_t GRAYCALL cMem::ConvertToString(char* pszDst, StrLen_t iSizeDstMax, const BYTE* pSrc, size_t nSrcQty) // static
	{
		//! Write bytes out to a string as comma separated base 10 number values. 
		//! Try to use SetHexDigest() instead.
		//! opposite of cMem::ReadFromString().
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

	size_t GRAYCALL cMem::ReadFromString(BYTE* pOut, size_t iDstMax, const char* pszSrc)
	{
		//! Read/Parse bytes in from string as comma separated base 10 number values. opposite of cMem::ConvertToString().
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

	StrLen_t GRAYCALL cMem::GetHexDigest(OUT char* pszHexString, const BYTE* pData, size_t nSizeData) // static 
	{
		//! Get the final hash as a pre-formatted string of hex digits. opposite of cMem::SetHexDigest
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

	HRESULT GRAYCALL cMem::SetHexDigest(const char* pszHexString, OUT BYTE* pData, size_t nSizeData) // static 
	{
		//! Set binary pDigest from string pszHexString
		//! opposite of cMem::GetHexDigest
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

	COMPARE_t GRAYCALL cMem::Compare(const void* pData1, size_t iLen1, const void* pData2, size_t iLen2) // static
	{
		//! @return COMPARE_Equal
		const size_t iLenMin = MIN(iLen1, iLen2);
		const COMPARE_t iRet = cMem::Compare(pData1, pData2, iLenMin);
		if (iRet != COMPARE_Equal)
			return iRet;
		return cValT::Compare(iLen1, iLen2);	// the longer one wins. if otherwise equal. (but not same length)
	}

	//**********************************************************

	void GRAYCALL cValArray::ReverseArrayBlocks(void* pArray, size_t nArraySizeBytes, size_t nBlockSize)
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
			cMem::Swap(pMemBS, pMemBE, nBlockSize);
		}
	}
}
 