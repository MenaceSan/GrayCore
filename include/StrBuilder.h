//
//! @file StrBuilder.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_StrBuilder_H
#define _INC_StrBuilder_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "StrT.h"
#include "cHeap.h"
#include "StrConst.h"

namespace Gray
{
	template< typename _TYPE_CH> class GRAYCORE_LINK StrFormat;	// Forward declare.

	template< typename _TYPE_CH = char >
	class GRAYCORE_LINK StrBuilder : protected cMemBlock //   GRAYCORE_LINK
	{
		//! @class Gray::StrBuilder
		//! Similar to .NET StringBuilder
		//! Fill a buffer with stuff. Track how much space is left. NOT dynamic resized.
		//! Like cQueue or cStreamOutput

		typedef cMemBlock SUPER_t;
		friend class StrFormat<_TYPE_CH>;
		friend class StrTemplate;

	public:
		StrLen_t m_iWriteLast;	//!< new items added/written here. end of readable.

	protected:
		inline _TYPE_CH* get_DataWork()
		{
			return (_TYPE_CH*)SUPER_t::get_DataV();
		}

	protected:
		inline StrLen_t get_AllocQty() const noexcept
		{
			return (StrLen_t)(SUPER_t::get_DataSize() / sizeof(_TYPE_CH));
		}

		inline void SetTerminated() noexcept
		{
			// always force terminate.
			if (!isValidPtr())	// just estimating.
				return;
			get_DataWork()[this->m_iWriteLast] = '\0';
		}

	public:
		inline StrLen_t get_WriteSpaceQty() const noexcept
		{
			//! How much space is avail for write into buffer? (given the current get_AllocQty() allocation size)
			//! free space to write more. Not including '\0'
			//! @return Qty of TYPE
			return (get_AllocQty() - this->m_iWriteLast) - 1;		// leave space for '\0'
		}

		virtual _TYPE_CH* GetWritePrepared(StrLen_t iNeedCount)
		{
			//! Do i have enough room to write iNeedCount of TYPE?
			//! Allocate as much as i can and truncate the rest.
			//! paired with AdvanceWrite
			UNREFERENCED_PARAMETER(iNeedCount);
			if (!isValidPtr())	// just estimating.
				return nullptr;
			return get_DataWork() + this->m_iWriteLast;
		}

		inline void AdvanceWrite(StrLen_t nLen) noexcept
		{
			DEBUG_CHECK(nLen <= get_WriteSpaceQty());
			this->m_iWriteLast += nLen;
			DEBUG_CHECK(this->m_iWriteLast >= 0);
			SetTerminated();
		}

	public:
		StrBuilder(_TYPE_CH* p, StrLen_t nSize) noexcept
			: cMemBlock(p, nSize * sizeof(_TYPE_CH))
			, m_iWriteLast(0)
		{
			SetTerminated();
		}

		StrBuilder(cMemBlock& m) noexcept
			: cMemBlock(m)
			, m_iWriteLast(0)
		{
			SetTerminated();
		}

		virtual ~StrBuilder()
		{
		}

		void SetEmptyStr() noexcept
		{
			m_iWriteLast = 0;
			SetTerminated();
		}
		inline StrLen_t get_Length() const noexcept
		{
			//! get Length used/filled. not including '\0';
			return m_iWriteLast;
		}
		inline const _TYPE_CH* get_Str() const noexcept
		{
			return (const _TYPE_CH*)SUPER_t::get_DataV();
		}
		inline bool isOverflow() const noexcept
		{
			//! Assume truncation occurred. DISP_E_BUFFERTOOSMALL
			return get_WriteSpaceQty() <= 0;
		}

		bool AddChar(_TYPE_CH ch)
		{
			// m_nLenLeft includes space for terminator '\0'
			_TYPE_CH* pszWrite = GetWritePrepared(1);
			const StrLen_t nLenRet = get_WriteSpaceQty();
			if (nLenRet < 1)	// no space.
				return false;
			if (pszWrite != nullptr)
			{
				*pszWrite = ch;
			}
			AdvanceWrite(1);
			return true;
		}
		bool AddNl()
		{
			// Add newline.
			return AddChar('\n');
		}
		bool AddNl2()
		{
			// Add newline ONLY if there is already text.
			if (m_iWriteLast <= 0)
				return false;
			return AddChar('\n');
		}

		StrLen_t AddCharRepeat(_TYPE_CH ch, StrLen_t iRepeat)
		{
			_TYPE_CH* pszWrite = GetWritePrepared(iRepeat);
			const StrLen_t nLenSpace = get_WriteSpaceQty();
			const StrLen_t nLenRet = MIN(iRepeat, nLenSpace);
			if (pszWrite != nullptr)
			{
				cValArray::FillQty<_TYPE_CH>(pszWrite, nLenRet, ch);
			}
			AdvanceWrite(nLenRet);
			return nLenRet;
		}

		StrLen_t AddStr2(const _TYPE_CH* pszStr, StrLen_t nLen)
		{
			// nLen = not including space for '\0'
			if (nLen <= 0)	// just add nothing.
				return 0;
			_TYPE_CH* pszWrite = GetWritePrepared(nLen);
			const StrLen_t nLenSpace = get_WriteSpaceQty();
			StrLen_t nLenRet = MIN(nLenSpace, nLen);
			if (pszWrite != nullptr)
			{
				nLenRet = StrT::CopyLen(pszWrite, pszStr, nLenRet + 1);	// add 1 more for '\0'
			}
			AdvanceWrite(nLenRet);
			return nLenRet;
		}

		StrLen_t AddStr(const _TYPE_CH* pszStr)
		{
			// AKA WriteString()
			return AddStr2(pszStr, StrT::Len(pszStr));
		}

		StrLen_t _cdecl Join(const _TYPE_CH* psz1, ...)
		{
			// Join a nullptr terminated list of strings. AKA Concat
			StrLen_t len = 0;
			va_list vargs;
			va_start(vargs, psz1);
			for (int i = 0; i < k_ARG_ARRAY_MAX; i++)
			{
				if (StrT::IsNullOrEmpty(psz1))
					break;
				const StrLen_t lenStr = AddStr(psz1);
				len += lenStr;
				psz1 = va_arg(vargs, const _TYPE_CH*);	// next 
			}
			va_end(vargs);
			return len;
		}
		void AddCrLf()
		{
			// AKA CRNL
			AddStr(cStrConst::k_CrLf);
		}

		void AddBytesRaw(const void* pSrc, size_t nSize)
		{
			// add raw bytes as chars with no filtering.
			_TYPE_CH* pszWrite = GetWritePrepared((StrLen_t)nSize);
			StrLen_t nLenRet = get_WriteSpaceQty();
			nLenRet = MIN(nLenRet, (StrLen_t)nSize);
			if (pszWrite != nullptr)
			{
				cMem::Copy(pszWrite, pSrc, nLenRet);
			}
			AdvanceWrite(nLenRet);
		}

		void AddBytesFiltered(const void* pSrc, size_t nSize)
		{
			// Just add a string from void*. Don't assume terminated string. filter for printable characters.
			_TYPE_CH* pszWrite = GetWritePrepared((StrLen_t)nSize);
			StrLen_t nLenRet = get_WriteSpaceQty();
			nLenRet = MIN(nLenRet, (StrLen_t)nSize);
			if (pszWrite != nullptr)
			{
				for (StrLen_t i = 0; i < nLenRet; i++)
				{
					BYTE ch = ((BYTE*)pSrc)[i];
					if (ch < 32 || ch == 127 || (ch > 128 && ch < 160))	// strip junk chars.
						pszWrite[i] = '?';
					else
						pszWrite[i] = ch;
				}
			}
			AdvanceWrite(nLenRet);
		}

		// StrLen_t AddIL(INT64 nVal, RADIX_t nBaseRadix = 10);
		// StrLen_t AddUL(UINT64 nVal, RADIX_t nBaseRadix = 10, char chRadixA = 'A');

		void _cdecl AddFormat(const _TYPE_CH* pszFormat, ...);
	};

	template< typename _TYPE_CH = char>
	class GRAYCORE_LINK StrBuilderDyn : public StrBuilder< _TYPE_CH> // GRAYCORE_LINK
	{
		//! @class Gray::StrBuilderDyn
		//! Grow string on demand to k_LEN_MAX.
		//! like cQueueDyn

		typedef StrBuilder< _TYPE_CH> SUPER_t;

	protected:
		static const StrLen_t k_nGrowSizeChunk = 256;

	protected:
		_TYPE_CH* GetWritePrepared(StrLen_t iNeedCount) override
		{
			// Get more space ?
			const StrLen_t nLenSpace = this->get_WriteSpaceQty();
			if (iNeedCount > nLenSpace)
			{
				// realloc for more space.

				const StrLen_t nOldAlloc = this->get_AllocQty();
				if (nOldAlloc < StrT::k_LEN_MAX) // we hit the end?
				{
					StrLen_t nNewAlloc = nOldAlloc + (iNeedCount - nLenSpace);	// Min size for new alloc.

					StrLen_t iRem = nNewAlloc % k_nGrowSizeChunk;
					if (iRem <= 0)
						iRem = k_nGrowSizeChunk;	// grow a full block.
					nNewAlloc += iRem;

					if (nNewAlloc > StrT::k_LEN_MAX) // we hit the end! do what we can. then truncate.
						nNewAlloc = StrT::k_LEN_MAX;

					nNewAlloc++;	// room for '\0'
					const size_t nAllocSize = nNewAlloc * sizeof(_TYPE_CH);
					this->SetBlock(cHeap::ReAllocPtr(this->get_DataV(), nAllocSize), nAllocSize);
				}
			}

			return SUPER_t::GetWritePrepared(iNeedCount);
		}

	public:
		StrBuilderDyn(StrLen_t nSizeStart = k_nGrowSizeChunk)
			: StrBuilder<_TYPE_CH>((_TYPE_CH*)cHeap::AllocPtr((size_t)(nSizeStart * sizeof(_TYPE_CH))), nSizeStart)
		{
		}
		virtual ~StrBuilderDyn()
		{
			cHeap::FreePtr(this->get_DataV());
		}
	};
}

#endif
