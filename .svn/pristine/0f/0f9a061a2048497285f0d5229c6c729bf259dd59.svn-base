//
//! @file CHeap.h
//! wrap a dynamically allocated (un-typed) block of heap memory.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CHeap_H
#define _INC_CHeap_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CMem.h"
#include "CDebugAssert.h"
#include "CUnitTestDecl.h"

// #define USE_HEAP_STATS		// Debug total allocation stats.
UNITTEST_PREDEF(CHeap)

namespace Gray
{
	struct GRAYCORE_LINK CHeap	// static class
	{
		//! @struct Gray::CHeap
		//! A static name space for applications main heap allocation/free related functions.
		//! @note Turning on the _DEBUG heap automatically uses _malloc_dbg()
		//! _DEBUG will put header and footer info on each heap allocation.

		enum FILL_TYPE
		{
			//! @enum Gray::CHeap::FILL_TYPE
			//! Debug Heap fill bytes.
			FILL_AllocStack = 0xCC,	//!< allocated on the stack in debug mode.
			FILL_Alloc = 0xCD,	//!< filled to indicate malloc() memory in debug mode.
			FILL_Freed = 0xDD,	//!< filled to indicate free() has been called on this.

#if (_MSC_VER<1400)
			FILL_AlignTail = 0xBD,	//!< Fills the m_TailGap _DEBUG ONLY
#else
			FILL_AlignTail = 0xED,	//!< Fills the m_TailGap _DEBUG ONLY
#endif
			FILL_UnusedStack = 0xFE,	//!< _DEBUG vsnprintf fills unused space with this.
			FILL_Prefix = 0xFD,	//!< Fills the gap before the returned memory block. _DEBUG ONLY
		};

		static const size_t k_ALLOC_MAX = 0x1000000; //!< 256 * 64K = (arbitrary) largest reasonable single malloc.

		static ITERATE_t sm_nAllocs;		//!< count total allocations (i.e. Number of calls to malloc() minus calls to free())
#ifdef USE_HEAP_STATS
		static size_t sm_nAllocTotalBytes;	//!< Keep running count of total memory allocated.
#endif

		static UINT64 GRAYCALL get_PhysTotal();		// For process/machine
		static UINT64 GRAYCALL get_PhysAvail();

		static void GRAYCALL Init(int nFlags = 0);
		static bool GRAYCALL Check();
		static size_t GRAYCALL GetSize(const void* pData);
		static bool GRAYCALL IsValidHeap(const void* pData);

		static inline bool IsCorruptHeap(const void* pData)
		{
			//! is this NOT a valid malloc() heap pointer?
			//! @note this should only ever be used in debug code. and only in an ASSERT.
			if (pData == nullptr)	// nullptr is not corrupt.
				return false;
			return !IsValidHeap(pData);
		}
		static bool GRAYCALL IsValidInside(const void* pData, ptrdiff_t index);

		static void* GRAYCALL AllocPtr(size_t nSize);
		static void* GRAYCALL AllocPtr(size_t nSize, const void* pDataInit);
		static void GRAYCALL FreePtr(void* pData);
		static void* GRAYCALL ReAllocPtr(void* pData, size_t nSize);

		UNITTEST_FRIEND(CHeap);
	};

	struct GRAYCORE_LINK CHeapAlign : public CHeap	// static
	{
		//! @struct Gray::CHeapAlign
		//! Allocate a block of memory that starts on a certain alignment. will/may have padded prefix.
		//! destruct = free memory.
		//! Linux might use posix_memalign() http://linux.about.com/library/cmd/blcmdl3_posix_memalign.htm
		//! size align must be a power of two and a multiple of sizeof(void *).
		typedef CHeap SUPER_t;

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
		static const size_t k_SizeGap = 4;
		static const size_t k_SizeAlignMax = 128;	// max reasonable size for alignment.

		struct CATTR_PACKED CHeader
		{
			//! @struct Gray::CHeapAlign::CHeader
			//! Aligned block of memory. allocated using _aligned_malloc or malloc.
			//! FROM MSVC.NET 2003 CRT - MAY NEED CHANGING FOR OTHER COMPILER
			//! ASSUME: alignment empty memory is here. contains 0x0BADF00D repeated.
			void* m_pMallocHead;	// pointer back to the returned malloc() memory. may point at self !
#ifdef _DEBUG
			BYTE m_TailGap[k_SizeGap];	//!< filled with HEAP_BYTE_AlignTailFill IN _DEBUG ONLY HEAP
#endif
										// ASSUME memory block follows this
		};

		static CHeader* GRAYCALL GetHeader(const void* pData);

		static bool GRAYCALL IsAlignedAlloc(const void* pData, size_t iAligned);
		static bool GRAYCALL IsValidHeap(const void* pData);
		static size_t GRAYCALL GetSize(const void* pData);
		static bool GRAYCALL IsValidInside(const void* pData, INT_PTR index);
		static void* GRAYCALL AllocPtr(size_t nSize, size_t iAligned);
		static void GRAYCALL FreePtr(void* pData);

#else
		// stub these out.
		static inline bool IsAlignedAlloc(const void* pData, size_t iAligned)
		{
			return false;
		}
		static inline bool IsValidHeap(const void* pData)
		{
			return SUPER_t::IsValidHeap(pData);
		}
		static inline size_t GetSize(const void* pData)
		{
			return SUPER_t::GetSize(pData);
		}
		static inline bool IsValidInside(const void* pData, INT_PTR index)
		{
			return SUPER_t::IsValidInside(pData, index);
		}
		static inline void* AllocPtr(size_t nSize, size_t iAligned)
		{
			return SUPER_t::AllocPtr(nSize);
		}
		static inline void FreePtr(void* pData)
		{
			return SUPER_t::FreePtr(pData);
		}

#endif // _MSC_VER
	};

	class GRAYCORE_LINK CHeapBlock : public CMemBlock
	{
		//! @class Gray::CHeapBlock
		//! A CMemBlock allocated using CHeap. Actual heap allocated size might be more than CMemBlock m_nSize in __linux__ or Lazy allocations.
		//! destruct = call CHeap::FreePtr().
		typedef CHeapBlock THIS_t;
		typedef CMemBlock SUPER_t;

	private:
		explicit CHeapBlock(void* pData)
		{
			//! never call this !
			UNREFERENCED_PARAMETER(pData);
			ASSERT(0);
		}

	public:
		CHeapBlock()
		{
			ASSERT(m_pData == nullptr);
			ASSERT(m_nSize == 0);
		}
		CHeapBlock(const THIS_t& ref)
		{
			//! copy constructor
			Alloc(ref.m_pData, ref.m_nSize);
		}
		CHeapBlock(THIS_t&& ref) noexcept
		{
			//! move constructor.
			m_pData = ref.m_pData; ref.m_pData = nullptr;
			m_nSize = ref.m_nSize; ref.m_nSize = 0;
		}
		explicit CHeapBlock(size_t nSize)
		{
			//! Construct with initial size. uninitialized data.
			Alloc(nSize);
		}
		CHeapBlock(const void* pDataCopy, size_t nSize)
		{
			//! Allocate then Copy pDataCopy data into this.
			Alloc(pDataCopy, nSize);
		}
		~CHeapBlock()
		{
			CHeap::FreePtr(m_pData);	// nullptr is OK/Safe here.
		}

		THIS_t& operator = (const THIS_t& ref)
		{
			//! copy assignment operator
			Alloc(ref.m_pData, ref.get_Size());
			return *this;
		}
		THIS_t& operator = (THIS_t&& ref)
		{
			//! move assignment operator
			m_pData = ref.m_pData; ref.m_pData = nullptr;
			m_nSize = ref.m_nSize; ref.m_nSize = 0;
			return *this;
		}

		bool isValidRead() const
		{
			//! Is this valid to use for read?
			//! Must NOT be nullptr!
			//! Has the memory been corrupted ?
			//! @note this should only ever be used in debug code. and only in an ASSERT.
			return CHeap::IsValidHeap(m_pData);
		}
		bool isCorrupt() const
		{
			//! Is this a corrupt heap pointer?
			//! nullptr is OK.
			//! @note this should only ever be used in debug code. and only in an ASSERT.
			return CHeap::IsCorruptHeap(m_pData);
		}

		size_t get_AllocSize() const
		{
			//! Special version of get_Size() to measure the true allocation size.
			//! @return The actual size of the allocation. May be greater than I requested? get_Size()
			//! @note Not always the size of the allocation request in __linux__ or Lazy.
			ASSERT(!isCorrupt());
			return CHeap::GetSize(m_pData);
		}
		size_t GetHeapStats(OUT ITERATE_t& iAllocCount) const
		{
			//! sizeof all children alloc(s). not size of *this
			if (!isValid())
				return 0;
			iAllocCount++;	// count total allocations used.
			return get_AllocSize();
		}
		void Free()
		{
			if (!isValid())
				return;
			CHeap::FreePtr(m_pData);
			SetEmptyBlock();
		}
		void FreeSecure()
		{
			if (!isValid())
				return;
			CMem::ZeroSecure(m_pData, get_Size());
			CHeap::FreePtr(m_pData);
			SetEmptyBlock();	// m_pData = nullptr
		}

		void SetHeapBlock(void* pData, size_t nSize)
		{
			//! Dangerous to allow anyone to poke a new pData pointer and nSize into this. 
			//! We will free pData on destructor!
			m_pData = pData;
			m_nSize = nSize;
		}
		void DetachHeapBlock()
		{
			//! Someone has copied this buffer.
			SetEmptyBlock();
		}
		bool Alloc(size_t nSize)
		{
			//! Allocate a memory block of size. assume m_pData points to uninitialized data.
			//! @note CHeap::AllocPtr(0) != nullptr ! maybe ?
			//! Some really old/odd code relies on AllocPtr(0) having/returning a real pointer? not well defined.
			CHeap::FreePtr(m_pData);
			if (nSize == 0)
			{
				m_pData = nullptr;
			}
			else
			{
				m_pData = CHeap::AllocPtr(nSize);
				if (!isValid())		// nSize = 0 may be nullptr or not?
				{
					return false;	// FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
				}
			}
			m_nSize = nSize;
			return true;	// nullptr is OK for size = 0
		}
		bool Alloc(const void* pData, size_t nSize)
		{
			//! Allocate then copy something into it.

			ASSERT(pData == nullptr || !this->IsValidPtr(pData));	// NOT from myself ! // Check before Alloc
			if (!Alloc(nSize))
			{
				return false;	// FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
			}
			if (pData != nullptr)
			{
				CMem::Copy(m_pData, pData, nSize);
			}
			return true;
		}
		bool ReAlloc(size_t nSize)
		{
			//! If already allocated re-use the current block if possible.
			//! copy existing data to new block if move is needed. preserve data.
			if (nSize != m_nSize)
			{
				m_pData = CHeap::ReAllocPtr(m_pData, nSize);
				if (nSize > 0 && !isValid())
				{
					return false;	// FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
				}
				m_nSize = nSize;
			}
			return true;
		}
		bool ReAlloc(const void* pData, size_t nSize)
		{
			ASSERT(pData == nullptr || !this->IsValidPtr(pData));	// NOT from myself ! // Check before Alloc
			if (!ReAlloc(nSize))
			{
				return false;	// FAILED HRESULT_WIN32_C(ERROR_NOT_ENOUGH_MEMORY)
			}
			if (pData != nullptr)
			{
				CMem::Copy(m_pData, pData, nSize);
			}
			return true;
		}

		bool ReAllocLazy(size_t iSizeNew)
		{
			//! Do not shrink the buffer size. only grow. but record the size i asked for.
			//! A HeapBlock that is faster to reallocate. optimize reallocate to smaller size by leaving the allocation alone. Lazy realloc in the case of shrink.
			if (iSizeNew > m_nSize && iSizeNew > get_AllocSize())
			{
				if (!ReAlloc(iSizeNew))
					return false;
			}
			m_nSize = iSizeNew;
			return true;
		}

		bool SetCopy(const CHeapBlock& rSrc)
		{
			//! Copy from h into me. 
			if (&rSrc == this)
				return true;
			return Alloc(rSrc.get_Data(), rSrc.get_Size());
		}

		void* get_Data() const
		{
			//! Might be nullptr. that's OK.
#ifdef _DEBUG
			ASSERT(!isCorrupt());
#endif
			return m_pData;	// get_Start();
		}
		BYTE* get_DataBytes() const
		{
			//! possibly nullptr.
			return((BYTE*)get_Data());
		}
		char* get_DataChars() const
		{
			return((char*)get_Data());
		}
		operator void* () const
		{
			return get_Data();
		}
		operator BYTE* () const
		{
			return get_DataBytes();	// for use with []
		}
		operator char* () const
		{
			return get_DataChars();	// for use with []
		}
	};
}

#endif	// _INC_CHeap_H
