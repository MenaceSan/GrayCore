//
//! @file cObject.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cObject_H
#define _INC_cObject_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "PtrCast.h"
#include "cDebugAssert.h"
#include "cMem.h"

namespace Gray
{
	class cArchive;

	template <UINT32 _SIGVALID = 0xCA11AB11>
	class DECLSPEC_NOVTABLE cObjectSignature : public cMemSignature<_SIGVALID>
	{
		//! @class Gray::cObjectSignature
		//! use this to make sure 2 DLL/SO's agree on the format and version of some object.

		typedef cMemSignature<_SIGVALID> SUPER_t;
	private:
		const UINT32 m_nVer;			//!< Must be agreed to by all users. e.g. _INC_GrayCore_H
		const size_t m_nSizeofThis;		//!< Must be agreed to by all users. sizeof(CLASS) for checking alignments of structures.

	public:
		cObjectSignature(UINT32 nVer, size_t nSizeofThis)
			: m_nVer(nVer), m_nSizeofThis(nSizeofThis)
		{
		}
		~cObjectSignature()
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
#define COBJECT_IsValidCheck()	cMem::IsValid(this,4)	// not in _MFC_VER.
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
		virtual ~CObject() noexcept 
		{
		}
		virtual bool isValidCheck() const noexcept	//!< memory allocation and structure definitions are valid.
		{
			//! NOT in MFC so use COBJECT_IsValidCheck to call.
			//! @note This can't be called in constructors and destructors of course !
			if (!cMem::IsValidApp(this))	// at least not null. (or near it)
			{
				DEBUG_CHECK(false);
				return false;
			}
			if (cMem::IsCorrupt(this, 4))	// no write privs ? _DEBUG only ?
			{
				DEBUG_CHECK(false);
				return false;
			}
			if (!IS_TYPE_OF(CObject, this))	// structure definitions are valid. use _CPPRTTI.
			{
				DEBUG_CHECK(false);
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

#ifndef _MFC_VER
	// Dynamic cObject is one that can be created knowing only its name and perhaps some interface that it supports. using cObjectFactoryT<T>
#define DECLARE_DYNAMIC(c)			//__noop
#define IMPLEMENT_DYNAMIC(c, cb)	//__noop
#endif // _MFC_VER

};

#endif
