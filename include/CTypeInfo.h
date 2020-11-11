//
//! @file cTypeInfo.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//! @todo cTypeInfo
//
#ifndef _INC_cTypeInfo_H
#define _INC_cTypeInfo_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "GrayCore.h"
#include "cUnitTestDecl.h"

#ifndef UNDER_CE
#include <typeinfo>		// type_info& typeid(class type) std::
#endif

UNITTEST_PREDEF(cTypeInfo)

namespace Gray
{
#ifdef UNDER_CE // no TYPEINFO_t in UNDER_CE. stub this out.
	typedef struct TYPEINFO_t { BYTE notused; };
#define typeid(x) 0			// stub this out.
#elif defined(_MSC_VER)
	typedef ::type_info TYPEINFO_t;		// Info from typeid()
#pragma warning(disable:4275)	// non dll-interface class 'type_info' used as base for dll-interface class. http://msdn.microsoft.com/en-us/library/3tdb471s.aspx 
#else // __GNUC__
	typedef std::type_info TYPEINFO_t;	// Info from typeid()
#endif

	class GRAYCORE_LINK cTypeInfo : public TYPEINFO_t
	{
		//! @class Gray::cTypeInfo
		//! Get reflection info about a C++ class that has virtuals and RTTI. Read its vtable. (_CPPRTTI)
		//! Supplement/helper for built in TYPEINFO_t via typeid()
		//! @note ASSUME TYPEINFO_t/type_info Always supports name(), hashcode() ?
		//! @todo cTypeInfo List of virtual members from vtable.

	public:
		size_t get_HashCode() const
		{
#ifdef _MSC_VER
			return hash_code();
#else
			return (size_t) this;
#endif
		}
		const char* get_Name() const
		{
			// Get the user friendly version of the name.
			return name();
		}

		const char* GetMemberName(int i) const;

		UNITTEST_FRIEND(cTypeInfo);
	};
}
#endif
