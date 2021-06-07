//
//! @file cMem.h
//! Move memory blocks and bytes around.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cMem_H
#define _INC_cMem_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Index.h"
#include "StrConst.h"
#include "cDebugAssert.h"

namespace Gray
{
	struct GRAYCORE_LINK cMem	// static cSingleton
	{
		//! @struct Gray::cMem
		//! a void type memory block. test bytes, Move memory bytes around.
		//! May be on heap, const memory space or static in stack. do NOT assume. use cHeap.

		static VOLATILE uintptr_t sm_bDontOptimizeOut0;	//!< static global byte to fool the optimizer into preserving this data.
		static VOLATILE uintptr_t sm_bDontOptimizeOutX;	//!< static global byte to fool the optimizer into preserving this data.
		static const size_t k_PageSizeMin = 64;		//!< Minimum page size for architecture. Usually More like 4K ?

#if ! defined(UNDER_CE) && ( ! defined(_DEBUG) || ! defined(_MSC_VER))
		static void __cdecl IsValidFailHandler(int nSig);	// JMP_t
#endif

		template <typename T>
		static constexpr const T* ToPtr(const void* p) noexcept
		{
			// Cast void pointer to a type.
			// put C26493 - "don't use c-style casts". warning in one place.
			return (const T*)p;
		}
		template <typename T>
		static constexpr T* ToPtr(void* p) noexcept
		{
			// Cast void pointer to a type.
			// put C26493 - "don't use c-style casts". warning in one place.
			return (T*)p;
		}

		static constexpr ptrdiff_t Diff(const void* pEnd, const void* pStart) noexcept
		{
			//! @return Difference in bytes. Assume it is a reasonable sized block? like GET_INDEX_IN()
			const ptrdiff_t i = ToPtr<BYTE>(pEnd) - ToPtr<BYTE>(pStart);	// like INT_PTR
			// DEBUG_CHECK(i > -(INT_PTR)(cHeap::k_ALLOC_MAX) && i < (INT_PTR)(cHeap::k_ALLOC_MAX));	// k_ALLOC_MAX
			return i;
		}

		static bool constexpr IsValidApp(const void* pData) noexcept
		{
			//! Is this pointer into App space? Not kernel space. Kernel Space <= 1G or 2G for __linux__
			//! Can i read from this ?
			//! Does not mean I have write permissions.
			//! Used to sanity check pointers. Ensure NOT offset from nullptr?

			if (pData == nullptr)
				return false;
			const UINT_PTR u = (UINT_PTR)pData;
			if (u < 16 * 1024)	// ASSUME memory in this range is never valid? Fail quickly. This is Kernel Space ONLY. <1G . PageSize ?
				return false;	// 
#ifdef _WIN32
			// 1G or 2G ?
#endif
			return true;
		}

		static bool GRAYCALL IsCorrupt(const void* pData, size_t nSize, bool bWriteAccess = false) noexcept;

		static bool inline IsValidPtr(const void* pData) noexcept
		{
			//! Can i read or write to this ?
			//! if DEBUG call IsCorrupt(pData,1) ??
			return IsValidApp(pData);
		}

		static constexpr bool IsInsideBlock(const void* p, const void* pBlock, size_t len) noexcept
		{
			const ptrdiff_t d = Diff(p, pBlock);
			return d >= 0 && d < (ptrdiff_t)len;
		}

		static inline bool IsZeros(const void* pData, size_t nSize) noexcept
		{
			//! Is all zeros ? nSize = 0 = true.
			if (!IsValidApp(pData))
				return true;
			for (size_t i = 0; i < nSize; i++)
			{
				if (ToPtr<BYTE>(pData)[i] != 0)
					return false;
			}
			return true;
		}

		static inline COMPARE_t Compare(const void* p1, const void* p2, size_t nSizeBlock) noexcept
		{
			//! Compare two blocks of memory. ASSUME both are at least nSizeBlock sized.
			//! Does not assume memory alignment.
#if USE_CRT
			return ::memcmp(p1, p2, nSizeBlock);
#elif defined(__GNUC__)
			return ::__builtin_memcmp(p1, p2, nSizeBlock);
#else
			const BYTE* p1B = ToPtr<BYTE>(p1);
			const BYTE* p2B = ToPtr<BYTE>(p2);
			for (size_t i = 0; i < nSizeBlock; i++)
			{
				register BYTE b1 = p1B[i];
				register BYTE b2 = p2B[i];
				if (b1 != b2)
					return b1 - b2;
			}
			return 0;
#endif
		}

		static COMPARE_t GRAYCALL Compare(const void* pData1, size_t iLen1, const void* pData2, size_t iLen2);

		static inline bool IsEqual(const void* p1, const void* p2, size_t nSizeBlock) noexcept
		{
			return cMem::Compare(p1, p2, nSizeBlock) == 0;
		}

		static inline COMPARE_t CompareSecure(const void* p1, const void* p2, size_t nSizeBlock) noexcept
		{
			//! constant-time buffer comparison. NOT efficient. Prevents timing based hacks.
			const BYTE* pB1 = ToPtr<BYTE>(p1);
			const BYTE* pB2 = ToPtr<BYTE>(p2);
			BYTE nDiff = 0;
			for (size_t i = 0; i < nSizeBlock; i++)
			{
				nDiff |= pB1[i] ^ pB2[i];
			}
			return (COMPARE_t)nDiff;
		}

		static size_t GRAYCALL CompareIndex(const void* p1, const void* p2, size_t nSizeBlock);

		static inline void Fill(void* pDst, size_t nSize, BYTE bVal) noexcept
		{
			//! Same as memset(). but with argument order change.
#if USE_CRT
			::memset(pDst, bVal, nSize);
#elif defined(__GNUC__)
			::__builtin_memset(pDst, bVal, nSize);
#elif defined(_WIN32)
			::FillMemory(pDst, nSize, bVal);
#else
			BYTE* pDstB = ToPtr<BYTE>(pDst);
			for (size_t i = 0; i < nSize; i++)
			{
				pDstB[j] = bVal;
			}
#endif
		}

		static inline void Zero(void* pData, size_t nSizeBlock) noexcept
		{
			//! Zero a block of memory.
			//! same as RtlZeroMemory() but maybe not the same as SecureZeroMeory() to make sure it is not optimized out. (for password clearing)
#ifdef _WIN32
			::ZeroMemory(pData, nSizeBlock);
#else
			Fill(pData, nSizeBlock, 0);
#endif
		}
		static inline void ZeroSecure(void* pData, size_t nSizeBlock) noexcept
		{
			//! This is for security purposes. Not for initialization. Zero destructed values so they leave no trace. 
			//! like SecureZeroMeory() and RtlSecureZeroMemory(). ensure it doesn't get optimized out. (like in an inline destructor)
			VOLATILE BYTE* p2 = ToPtr<BYTE>(pData);	// 'volatile' will ensure it doesn't get optimized out.
			while (nSizeBlock--)
				*p2++ = 0;
		}

		static inline void Xor(BYTE* pDst, const BYTE* pSrc, size_t nBlockSize) noexcept
		{
			// Xor with self.
			for (size_t i = 0; i < nBlockSize; i++)
				pDst[i] ^= pSrc[i];
		}

		static inline void Xor2(BYTE* pDst, const BYTE* pSrc1, const BYTE* pSrc2, size_t nBlockSize) noexcept
		{
			for (size_t i = 0; i < nBlockSize; i++)
				pDst[i] = pSrc1[i] ^ pSrc2[i];
		}

		static inline void Copy(void* pDst, const void* pSrc, size_t nSizeBlock) noexcept
		{
			//! Copy a block of memory.
			//! @arg nSizeBlock = if 0 then nullptr is ok.
			//! same as CopyMemory(), RtlCopyMemory, memcpy()
			//! @note: Some older architectures needed versions of this to do 'huge' memory copies.
#if USE_CRT
			::memcpy(pDst, pSrc, nSizeBlock);
#elif defined(__GNUC__)
			::__builtin_memcpy(pDst, pSrc, nSizeBlock);
#elif defined(_WIN32)
			::CopyMemory(pDst, pSrc, nSizeBlock);
#else
			register BYTE* pDstB = ToPtr<BYTE>(pDst);
			register const BYTE* pSrcB = ToPtr<BYTE>(pSrc);
			for (size_t i = 0; i < nSizeBlock; i++)
			{
				pDstB[i] = pSrcB[i];
			}
#endif
		}
		static inline void CopyOverlap(void* pDst, const void* pSrc, size_t nSizeBlock) noexcept
		{
			//! Copy possibly overlapping blocks of memory. start from end or beginning if needed.
			//! same as MoveMemory RtlMoveMemory, memmove, hmemcpy(),
			//! @note: Some older architectures needed versions of this to do 'huge' memory moves.
#if USE_CRT
			::memmove(pDst, pSrc, nSizeBlock);
#elif defined(_WIN32)
			::MoveMemory(pDst, pSrc, nSizeBlock);
#else
			register BYTE* pDstB = ToPtr<BYTE>(pDst);
			register const BYTE* pSrcB = ToPtr<BYTE>(pSrc);
			if (pDstB <= pSrcB || pDstB >= (pSrcB + nSizeBlock))
			{
				// Non-Overlapping Buffers. copy from lower addresses to higher addresses
				Copy(pDst, pSrc, nSizeBlock);
			}
			else
			{
				// Overlapping Buffers. copy from higher addresses to lower addresses
				pDstB += count - 1;
				pSrcB += count - 1;
				while (count--)
				{
					*pDstB = *pSrcB;
					pDstB--;
					pSrcB--;
				}
			}
#endif
		}
		static inline void ReverseBytes(void* pDst, size_t nSizeBlock) noexcept
		{
			register BYTE* pSrcB = ToPtr<BYTE>(pDst);
			register BYTE* pDstB = ToPtr<BYTE>(pDst) + nSizeBlock - 1;
			nSizeBlock /= 2;
			while (nSizeBlock--)
			{
				cValT::Swap(*pSrcB++, *pDstB--);
			}
		}
		static inline void CopyReverse(void* pDst, const void* pSrc, size_t nSizeBlock) noexcept
		{
			//! Copy a block of memory BYTEs reversed. e.g. {3,2,1} = {1,2,3}, nSizeBlock = 3
			if (pDst == pSrc)
			{
				ReverseBytes(pDst, nSizeBlock);
			}
			else
			{
				register BYTE* pDstB = ToPtr<BYTE>(pDst) + nSizeBlock - 1;
				register const BYTE* pSrcB = ToPtr<BYTE>(pSrc);
				while (nSizeBlock--)
				{
					*pDstB-- = *pSrcB++;
				}
			}
		}

		static inline void CopyRepeat(void* pDst, size_t nDstSize, const void* pSrc, size_t nSrcSize) noexcept
		{
			//! Fill pDst with repeating copies of pSrc. Wrap back to sart.
			for (size_t i = 0; i < nDstSize;)
			{
				register BYTE* pDstB = ToPtr<BYTE>(pDst);
				register const BYTE* pSrcB = ToPtr<BYTE>(pSrc);
				for (size_t j = 0; j < nSrcSize && i < nDstSize; j++, i++)
				{
					pDstB[i] = pSrcB[j];
				}
			}
		}

		static inline void CopyHtoN(BYTE* pDst, const void* pSrc, size_t nSizeBlock) noexcept
		{
			//! Copy from Host (Local Native) Order into Network Order (Big Endian)
#ifdef USE_LITTLE_ENDIAN
			cMem::CopyReverse(pDst, pSrc, nSizeBlock);
#else
			cMem::Copy(pDst, pSrc, nSizeBlock);
#endif
		}
		static inline void CopyNtoH(void* pDst, const BYTE* pSrc, size_t nSizeBlock) noexcept
		{
			//! Copy from Network Order (Big Endian) to Host Order (Local Native)
#ifdef USE_LITTLE_ENDIAN
			cMem::CopyReverse(pDst, pSrc, nSizeBlock);
#else
			cMem::Copy(pDst, pSrc, nSizeBlock);
#endif
		}

		static inline void Swap(void* pvMem1, void* pvMem2, size_t nBlockSize) noexcept
		{
			//! swap copy 2 blocks of memory by bytes. like cMemT::Swap() but for 2 arbitrary sized blocks.
			//! swap them byte by byte.
			//! use cMemT::Swap<> instead if possible for intrinsic types.
			//! @arg nBlockSize = size in bytes.
			//! @note
			//!  DO NOT Use this for complex types that need special copiers. have pointers and such. use cMemT::Swap()

			if (pvMem1 == pvMem2)	// no change.
				return;
			BYTE* pMem1 = ToPtr<BYTE>(pvMem1);
			BYTE* pMem2 = ToPtr<BYTE>(pvMem2);
			for (; nBlockSize--; pMem1++, pMem2++)
			{
				cValT::Swap(*pMem1, *pMem2);
			}
		}

		// read/write a string of comma separated numbers.
		static StrLen_t GRAYCALL ConvertToString(char* pszDst, StrLen_t iSizeDstMax, const BYTE* pSrc, size_t nSrcQty);
		static StrLen_t GRAYCALL ConvertToString(wchar_t* pszDst, StrLen_t iSizeDstMax, const BYTE* pSrc, size_t nSrcQty);
		static size_t GRAYCALL ReadFromString(BYTE* pDst, size_t iLenBytesMax, const char* pszSrc);

		static inline StrLen_t GetHexDigestSize(size_t nSize) noexcept
		{
			//!< How much space does the hex digest need?
			return (StrLen_t)((nSize * 2) + 1);
		}
		static StrLen_t GRAYCALL GetHexDigest(OUT char* pszHexString, const BYTE* pData, size_t nSizeData);
		static HRESULT GRAYCALL SetHexDigest(const char* pszHexString, OUT BYTE* pData, size_t nSizeData, bool testEnd = true);
	};

	template <UINT32 _SIGVALID = 0xCA11AB1E>
	class DECLSPEC_NOVTABLE cMemSignature
	{
		//! @class Gray::cMemSignature
		//! An unlikely pattern of debug data for validating the heap/memory. These may be scattered about any place.
		//! @note May not be allocated on the system heap.
		//! @note This is somewhat redundant with some built in _MSC_VER _DEBUG heap code. heap has its own mechanism to do this.
	public:
		static const UINT32 k_VALID = _SIGVALID;	//!< used just to make sure this is valid. FEEDF00D
		static const UINT32 k_INVALID = 0xDEADBEA7;	//!< Mark as NOT valid when freed!
	private:
		UINT32 m_nSignature;	//!< Hold bytes of known value.
	public:
		cMemSignature() noexcept
			: m_nSignature(_SIGVALID)
		{
			//! @note virtuals don't work in constructors or destructors !
		}
		~cMemSignature()
		{
			//! @note virtuals don't work in constructors or destructors !
			//! so ASSERT( isValidCheck()); not possible here !
			ASSERT(isValidSignature());
			m_nSignature = k_INVALID;	// Mark as invalid.
		}
		bool inline isValidSignature() const noexcept
		{
			if (!cMem::IsValidApp(this))
				return false;
			if (m_nSignature != _SIGVALID)
				return false;
			return true;
		}
	};

	template<size_t TYPE_SIZE>
	class cMemStaticSized
	{
		//! @class Gray::cMemStaticSized
		//! Store an inline/static allocated blob of a specific known size/type. TYPE_SIZE in bytes.

	public:
		static const size_t k_Size = TYPE_SIZE;	//!< All hashes of this type are this size (bytes).

		BYTE m_Data[TYPE_SIZE];		//!< All objects of this type are this size.

	public:
		size_t get_DataLength() const noexcept
		{
			//! size in bytes.
			return TYPE_SIZE;
		}

		const BYTE* get_DataBytes() const noexcept
		{
			return m_Data;
		}
		operator const BYTE* () const noexcept
		{
			return m_Data;
		}
	};

	class GRAYCORE_LINK cMemBlock
	{
		//! @class Gray::cMemBlock
		//! A pointer to memory block/blob with unknown ownership. may be heap, stack or const. don't free on destruct.
		//! May be static init?

	protected:
		size_t m_nSize;		//!< size_t of m_pData in bytes. May be determined at runtime.
		void* m_pData;

	public:
		static const cMemBlock k_EmptyBlock;

	public:
		cMemBlock() noexcept
			: m_nSize(0)
			, m_pData(nullptr)
		{
		}
		cMemBlock(const void* pData, size_t nSize) noexcept
			: m_nSize(nSize)
			, m_pData(const_cast<void*>(pData))	// just assume we don't modify it?
		{
		}
		cMemBlock(const cMemBlock& block) noexcept
			: m_nSize(block.m_nSize)
			, m_pData(block.m_pData)
		{
			// Just shared pointers. This may be dangerous!
		}
		cMemBlock(const cMemBlock* pBlock) noexcept
			: m_nSize((pBlock == nullptr) ? 0 : pBlock->m_nSize)
			, m_pData((pBlock == nullptr) ? nullptr : pBlock->m_pData)
		{
			// Just shared pointers. This may be dangerous!
		}

		inline size_t get_DataSize() const noexcept
		{
			return m_nSize;
		}

		inline void* get_DataV() const noexcept
		{
			//! get as void pointer. Might be nullptr. that's OK.
			return m_pData;
		}
		template <typename TYPE> 
		inline TYPE* get_Data()
		{
			//! get as TYPE pointer. Might be nullptr. that's OK.
			return (TYPE*)m_pData;
		}

		inline BYTE* get_DataBytes() const noexcept
		{
			//! Get as a BYTE pointer.
			return (BYTE*)m_pData;
		}
		inline char* get_DataA() const noexcept
		{
			//! Get as a char pointer.
			return (char*)m_pData;
		}
		 

		operator void* () const noexcept
		{
			return get_DataV();
		}
		operator const BYTE* () const noexcept
		{
			//! Get as a BYTE pointer.
			return get_DataBytes();
		}

		inline bool isValidPtr() const noexcept
		{
			//! Is this (probably) valid to use/read/write. not nullptr.
			return cMem::IsValidPtr(m_pData);
		}
		inline bool IsValidIndex(size_t i) const noexcept
		{
			//! Is i inside the known valid range for the block?
			return IS_INDEX_GOOD(i, m_nSize);
		}
		bool IsValidIndex2(size_t i) const noexcept
		{
			//! Is i inside the known valid range for the block? or at end?
			if (i == m_nSize)	// at end is ok
				return true;
			return IS_INDEX_GOOD(i, m_nSize);
		}

		inline bool IsInternalPtr(const void* p) const noexcept
		{
			//! Is p inside the known valid range for the block? Inclusive = Can be equal to end.
			return IsValidIndex(cMem::Diff(p, get_DataV()));
		}
		inline bool IsInternalPtr2(const void* p) const noexcept
		{
			//! Is p inside the known valid range for the block? Exclusive = Cant be equal to end!
			return IsValidIndex2(cMem::Diff(p, get_DataV()));
		}

		bool IsZeros() const noexcept
		{
			return cMem::IsZeros(m_pData, m_nSize);
		}

		bool IsEqualData(const void* pData, size_t nSize) const noexcept
		{
			//! compare blocks of data.
			return m_nSize == nSize && cMem::IsEqual(m_pData, pData, nSize);
		}
		bool IsEqualData(const cMemBlock* pData) const noexcept
		{
			//! compare blocks of data.
			if (pData == nullptr)
				return false;	// isValidPtr() ?
			return IsEqualData(pData->m_pData, pData->m_nSize);
		}
		bool IsEqualData(const cMemBlock& data) const noexcept
		{
			//! compare blocks of data.
			return IsEqualData(data.m_pData, data.m_nSize);
		}

		//! Compares an static string to a value in cMemBlock
		//! @note ONLY works for literal "static string", or BYTE[123] you cannot use a 'BYTE* x' here!
#define IsEqualLiteral(x) IsEqualData( STRLIT2(x))

		BYTE* GetSpan1(size_t nOffset) const noexcept
		{
			// get pointer that is good for just one byte.
			DEBUG_CHECK(IsValidIndex(nOffset));
			return get_DataBytes() + nOffset;
		}
		BYTE* GetSpan(size_t nOffset, size_t size) const noexcept
		{
			//! Get a pointer into the buffer as a byte pointer.
			//! Ensure the data is valid !
			DEBUG_CHECK(IsValidIndex(nOffset));
			DEBUG_CHECK(IsValidIndex2(nOffset + size));
			UNREFERENCED_PARAMETER(size);	// debug only.
			return get_DataBytes() + nOffset;
		}
		const void* get_DataEnd() const noexcept
		{
			//! Never write to this pointer.
			return get_DataBytes() + m_nSize;
		}

		void put_DataPtr(void* pStart) noexcept
		{
			//! Set Data pointer but leave size.
			m_pData = pStart;
			// ASSERT(is pStart reasonable?)
		}
		void put_DataSize(size_t nSize) noexcept
		{
			//! Set size but leave data pointer.
			m_nSize = nSize;
		}

		void SetBlock(void* pData, size_t nSize) noexcept
		{
			m_pData = pData;
			m_nSize = nSize;	// size does not apply if nullptr.
		}
		void SetEmptyBlock() noexcept
		{
			m_pData = nullptr;
			m_nSize = 0;
		}

		void InitZeros() noexcept
		{
			cMem::ZeroSecure(m_pData, m_nSize);
		}

	};

	struct GRAYCORE_LINK cMemT : public cValT	// Value of some type in memory.
	{
		//! @struct Gray::cMemT
		//! collection of templates to handle An arbitrary value type in memory. cValT in heap, const or stack.
		//! Deal with it as an array of bytes.

		template <class TYPE>
		static inline TYPE ReverseType(TYPE nVal) noexcept
		{
			//! Reverse the byte order in an intrinsic type.
			//! Like __GNUC__ __builtin_bswap16(), __builtin_bswap32, etc
			//! Similar to: ntohl(), htonl(), ntohs(), htons().

			cMem::CopyReverse(&nVal, &nVal, sizeof(nVal));
			return nVal;
		}
		template <typename TYPE>
		static inline TYPE HtoN(TYPE nVal) noexcept
		{
			//! Host byte order to network order (big endian). like htonl() htons()
#ifdef USE_LITTLE_ENDIAN
			//! Assume bytes are correct bit order but larger types are not ordered correctly.
			return ReverseType(nVal);
#else
			return nVal;	// no change needed.
#endif
		}
		template <typename TYPE>
		static inline TYPE NtoH(TYPE nVal) noexcept
		{
			//! Network byte order (big endian) to host order. like ntohl() ntohs()
			//! Network order = BigEndian = High order comes first = Not Intel.
#ifdef USE_LITTLE_ENDIAN
			//! Assume bytes are correct bit order but larger types are not ordered correctly.
			return ReverseType(nVal);
#else
			return nVal;	// no change needed.
#endif
		}

		template <typename TYPE>
		static inline TYPE HtoLE(TYPE nVal) noexcept
		{
			//! Host byte order to little endian. (Intel)
#ifdef USE_LITTLE_ENDIAN
			return nVal;	// no change needed.
#else
			//! Assume bytes are correct bit order but larger types are not ordered correctly.
			return ReverseType(nVal);
#endif
		}
		template <typename TYPE>
		static inline TYPE LEtoH(TYPE nVal) noexcept
		{
			//! Little Endian (Intel) to host byte order.
#ifdef USE_LITTLE_ENDIAN
			return nVal;	// no change needed.
#else
			//! Assume bytes are correct bit order but larger types are not ordered correctly.
			return ReverseType(nVal);
#endif
		}

		template <typename TYPE>
		static inline TYPE GetUnaligned(const void* pData) noexcept
		{
			//! Get a data value from an unaligned TYPE pointer.
			//! Like the _WIN32 UNALIGNED macro.
			//! @note some architectures will crash if you try to access unaligned data. (PowerPC)
			//! In this case we need to memcpy() to a temporary buffer first.
#ifdef _MSC_VER
			return *((const UNALIGNED TYPE*) pData);
#else
			return *((const TYPE*)pData);
#endif
		}
		template <typename TYPE>
		static inline void SetUnaligned(void* pData, TYPE nVal) noexcept
		{
			//! Get a data value from an unknown unaligned TYPE pointer.
			//! Like the _WIN32 UNALIGNED macro.
			//! @note some architectures will crash if you try to access unaligned data. (PowerPC)
			//! In this case we need to memcpy() to a temporary buffer first.
#ifdef _MSC_VER
			* ((UNALIGNED TYPE*) pData) = nVal;
#else
			* ((TYPE*)pData) = nVal;
#endif
		}

		template <typename TYPE>
		static inline TYPE GetLEtoH(const void* pData) noexcept
		{
			//! Get bytes packed as LE (Intel). ( Not "Network order" which is big endian.)
			return LEtoH(GetUnaligned<TYPE>(pData));
		}
		template <typename TYPE>
		static inline void SetHtoLE(void* pData, TYPE nVal) noexcept
		{
			//! Set bytes packed as LE (Intel). ( Not "Network order" which is big endian.)
			return SetUnaligned(pData, HtoLE(nVal));
		}
		template <typename TYPE>
		static inline TYPE GetNtoH(const void* pData) noexcept
		{
			//! Get bytes packed as BE (Network order, Not Intel).
			//! similar to CopyNtoH()
			return NtoH(GetUnaligned<TYPE>(pData));
		}
		template <typename TYPE>
		static inline void SetHtoN(void* pData, TYPE nVal) noexcept
		{
			//! Set bytes packed as BE (Network order, Not Intel).
			//! similar to CopyHtoN()
			return SetUnaligned(pData, HtoN(nVal));
		}

		static inline DWORD GetNVal3(const BYTE* p) noexcept
		{
			//! Get 3 packed BYTEs as a host value. from Network order. Big Endian.
			//! opposite of SetNVal3()
			return ((DWORD)p[0]) << 16 | ((DWORD)p[1]) << 8 | p[2];
		}
		static inline void SetNVal3(BYTE* p, size_t nVal) noexcept
		{
			//! Set 3 packed BYTEs as a value. Network order. Big Endian.
			//! opposite of GetNVal3()
			p[0] = (BYTE)((nVal >> 16) & 0xFF);
			p[1] = (BYTE)((nVal >> 8) & 0xFF);	// HIBYTE
			p[2] = (BYTE)((nVal) & 0xFF);	// LOBYTE
		}
	};

	template <>
	inline WORD cMemT::ReverseType<WORD>(WORD nVal) noexcept // static
	{
		//! Reverse the bytes in an intrinsic 16 bit type WORD. e.g. 0x1234 = 0x3412
		//! like ntohs(),htons(), MAKEWORD()
		return (WORD)((nVal >> 8) | (nVal << 8));
	}
	template <>
	inline UINT32 cMemT::ReverseType<UINT32>(UINT32 nVal) noexcept // static
	{
		//! Reverse the bytes in an intrinsic 32 bit type UINT32.
		//! like ntohl(),htonl()
		nVal = (nVal >> 16) | (nVal << 16);
		return ((nVal & 0xff00ff00UL) >> 8) | ((nVal & 0x00ff00ffUL) << 8);
	}

#ifdef USE_INT64
	template <>
	inline UINT64 cMemT::ReverseType<UINT64>(UINT64 nVal) noexcept // static
	{
		//! Reverse the bytes in an intrinsic 64 bit type UINT64.
		nVal = (nVal >> 32) | (nVal << 32);
		nVal = ((nVal & 0xff00ff00ff00ff00ULL) >> 8) | ((nVal & 0x00ff00ff00ff00ffULL) << 8);
		return ((nVal & 0xffff0000ffff0000ULL) >> 16) | ((nVal & 0x0000ffff0000ffffULL) << 16);
	}
#endif
#ifndef USE_LONG_AS_INT64
	template <>
	inline ULONG cMemT::ReverseType<ULONG>(ULONG nVal) noexcept // static
	{
		//! ULONG may be equiv to UINT32 or UINT64
		// return ReverseType<UINT64>(nVal);
		return ReverseType<UINT32>(nVal);
	}
#endif

#if 0 // USE_LITTLE_ENDIAN
	template <>
	inline UINT32 cMemT::GetNtoH<UINT32>(const void* pData)
	{
		const BYTE* p = (const BYTE*)pData;
		return ((UINT32)p[0] << 24)
			| ((UINT32)p[1] << 16)
			| ((UINT32)p[2] << 8)
			| ((UINT32)p[3]);
	}
	template <>
	inline void cMemT::SetHtoN<UINT32>(void* pData, UINT32 nVal)
	{
		BYTE* p = (BYTE*)pData;
		p[0] = (BYTE)(nVal >> 24);
		p[1] = (BYTE)(nVal >> 16);
		p[2] = (BYTE)(nVal >> 8);
		p[3] = (BYTE)(nVal);
	}

	template <>
	inline UINT64 cMemT::GetNtoH<UINT64>(const void* pData)
	{
		const BYTE* p = (const BYTE*)pData;
		return ((UINT64)p[0] << 56)
			| ((UINT64)p[1] << 48)
			| ((UINT64)p[2] << 40)
			| ((UINT64)p[3] << 32)
			| ((UINT64)p[4] << 24)
			| ((UINT64)p[5] << 16)
			| ((UINT64)p[6] << 8)
			| ((UINT64)p[7]);
	}
	template <>
	inline void cMemT::SetHtoN<UINT64>(void* pData, UINT64 nVal)
	{
		BYTE* p = (BYTE*)pData;
		p[0] = (BYTE)(nVal >> 56);
		p[1] = (BYTE)(nVal >> 48);
		p[2] = (BYTE)(nVal >> 40);
		p[3] = (BYTE)(nVal >> 32);
		p[4] = (BYTE)(nVal >> 24);
		p[5] = (BYTE)(nVal >> 16);
		p[6] = (BYTE)(nVal >> 8);
		p[7] = (BYTE)(nVal);
	}
#endif

}
#endif
