//
//! @file CObject.h
//! @copyright 1992 - 2016 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CObject_H
#define _INC_CObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "Ptr.h"
#include "CDebugAssert.h"
#include "CMem.h"

namespace Gray
{
	class cArchive;

	template <UINT32 _SIGVALID = 0xCA11AB11>
	class DECLSPEC_NOVTABLE CObjectSignature : public CMemSignature<_SIGVALID>
	{
		//! @class Gray::CObjectSignature
		//! use this to make sure 2 DLL/SO's agree on the format and version of some object.

		typedef CMemSignature<_SIGVALID> SUPER_t;
	private:
		const UINT32 m_nVer;			//!< Must be agreed to by all users. e.g. _INC_GrayCore_H
		const size_t m_nSizeofThis;		//!< Must be agreed to by all users. sizeof(CLASS) for checking alignments of structures.

	public:
		CObjectSignature(UINT32 nVer, size_t nSizeofThis)
			: m_nVer(nVer), m_nSizeofThis(nSizeofThis)
		{
		}
		~CObjectSignature()
		{
		}

		bool inline IsValidSignature(UINT32 nVer, size_t nSizeofThis) const
		{
			//! Call this from the context of some other DLL or lib and make sure they match.
			//! If not, then there are un-matching pound defines (conditional code) or different compiler packing params. This is BAD.
			//! Make sure this is inline compiled.

			if (!SUPER_t::isValidSignature())
				return false;
			if (nVer != m_nVer || nSizeofThis != m_nSizeofThis)
				return false;
			return true;
		}
	};

#if defined(_MFC_VER)
#define COBJECT_IsValidCheck()	CMem::IsValid(this,4)	// not in _MFC_VER.
#else

#ifdef _DEBUG
#define ASSERT_VALID(p)			(p)->AssertValid()		// Emulate MFC
#else
#define ASSERT_VALID(p)			ASSERT((p)!=nullptr)
#endif

#define COBJECT_IsValidCheck()	CObject::isValidCheck()	// check stuff but no ASSERT

	class GRAYCORE_LINK CObject
	{
		//! @class Gray::CObject
		//! Generic base class of all stuff. May be used to replace/emulate _CPPRTTI?
		//! Emulate the MFC CObject base class.
		//! May be base for stack or heap allocated object.

	public:
		virtual ~CObject()
		{
		}
		virtual bool isValidCheck() const	//!< memory allocation and structure definitions are valid.
		{
			//! NOT in MFC so use COBJECT_IsValidCheck to call.
			//! @note This can't be called in constructors and destructors of course !
			if (!CMem::IsValid(this, 4))	// at least not null. (or near it)
			{
				DEBUG_ASSERT(0, "isValidCheck");
				return false;
			}
			if (!IS_TYPE_OF(CObject, this))	// structure definitions are valid..
			{
				DEBUG_ASSERT(0, "isValidCheck");
				return false;
			}
			return true;
		}
		virtual void AssertValid() const	//!< memory allocation and structure definitions are valid.
		{
			//! MFC equivalent = virtual void AssertValid() const;
			ASSERT(isValidCheck());
		}

		virtual void Serialize(cArchive& a);	// Emulate MFC method.
	};
#endif // _MFC_VER

};

#endif
