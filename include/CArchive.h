//
//! @file CArchive.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CArchive_H
#define _INC_CArchive_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CStream.h"

UNITTEST_PREDEF(cArchive)

namespace Gray
{
	class GRAYCORE_LINK cArchive
	{
		//! @class Gray::cArchive
		//! Form a bidirectional (typeless) binary stream of serialized data.
		//! @note this is inherently dangerous to use since it contains no default/automatic typing/versioning information.
		//! @note put CVariant into the archive if you desire typing information. (and some version change resistance)
		//! This is extensible to any type.
		//! Similar to the MFC CArchive type. except << >> are overridden by the type of the Archive (store vs retrieve).
		//! i.e. store and retrieve a particular structure can use the same code.
	private:
		const bool m_bStoring;		//!< What mode is this in? true = writing to the m_pStream CStreamOutput. else reading CStreamInput.
		CStreamBase* const m_pStream;	//!< CStreamInput or CStreamOutput depending on m_bStoring

	public:
		cArchive(CStreamOutput& so) noexcept
			: m_bStoring(true)
			, m_pStream(&so)
		{
		}
		cArchive(CStreamInput& si) noexcept
			: m_bStoring(false)
			, m_pStream(&si)
		{
		}
		cArchive(CStream& s, bool bStoring) noexcept
			: m_bStoring(bStoring)
			, m_pStream(bStoring ? static_cast<CStreamBase*>(static_cast<CStreamOutput*>(&s)) : static_cast<CStreamBase*>(static_cast<CStreamInput*>(&s)))
		{
		}

		bool IsStoring() const noexcept
		{
			//! I am storing the object to the write archive CStreamOutput.
			//! like MFC CArchive 
			return m_bStoring;
		}
		bool IsLoading() const noexcept
		{
			//! I am loading the object from the read archive CStreamInput.
			//! like MFC CArchive 
			return !m_bStoring;
		}

		CStreamOutput& ref_Out()
		{
			ASSERT(IsStoring());
			return *static_cast<CStreamOutput*>(m_pStream);
		}
		CStreamInput& ref_Inp()
		{
			ASSERT(IsLoading());
			return *static_cast<CStreamInput*>(m_pStream);
		}

		//! Serialize Base Types
		HRESULT Serialize(void* pData, size_t nSize);
		HRESULT SerializeSize(size_t& nSize);

		HRESULT Write(const void* pData, size_t nSize)
		{
			// Emulate MFC. Insert into the archive.
			ASSERT(IsStoring());
			return Serialize(const_cast<void*>(pData), nSize);
		}
		HRESULT Read(void* pData, size_t nSize)
		{
			// Emulate MFC. Extract from the archive.
			ASSERT(IsLoading());
			return Serialize(pData, nSize);
		}

		size_t ReadCount()
		{
			// Emulate MFC
			ASSERT(IsLoading());
			size_t n;
			SerializeSize(n);
			return n;
		}
		void WriteCount(size_t n)
		{
			// Emulate MFC
			ASSERT(IsStoring());
			SerializeSize(n);
		}

#define CTYPE_DEF(a,_TYPE,c,d,e,f,g,h) HRESULT Serialize( _TYPE& Val ) { return Serialize(&Val,sizeof(Val)); } \
		cArchive& operator << (const _TYPE& Val) { Write(&Val,sizeof(Val)); return *this; } \
		cArchive& operator >> (_TYPE& Val) { Read(&Val,sizeof(Val)); return *this; } 

#include "CTypes.tbl"
#undef CTYPE_DEF

		UNITTEST_FRIEND(cArchive);
	};
}

#ifndef _MFC_VER	// emulate MFC.
typedef Gray::cArchive CArchive;
// #define CArchive Gray::cArchive 
#define DECLARE_SERIAL(class_name) friend CArchive& GRAYCALL operator>>(CArchive& ar, class_name* &pOb);	//   _DECLARE_DYNCREATE(class_name)  
#define IMPLEMENT_SERIAL(class_name,base_name,quant)	// external to class.
#endif

#endif
