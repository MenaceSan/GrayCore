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

#ifndef UNDER_CE
#include <typeinfo>		// type_info& typeid(class type) std:: 
#endif

namespace Gray
{

#ifdef UNDER_CE // no TYPEINFO_t in UNDER_CE. stub this out.
	typedef struct TYPEINFO_t { BYTE notused; };
#define typeid(x) 0			// stub this out.
#elif defined(_MSC_VER)
	typedef ::type_info TYPEINFO_t;		// Info from typeid(TYPE)
#pragma warning(disable:4275)	// non dll-interface class 'type_info' used as base for dll-interface class. http://msdn.microsoft.com/en-us/library/3tdb471s.aspx 
#else // __GNUC__
	typedef std::type_info TYPEINFO_t;	// Info from typeid(TYPE)
#endif

#define GETTYPEINFO(TYPE) ((const cTypeInfo&) typeid(TYPE)) // 

	class GRAYCORE_LINK cTypeInfo : public TYPEINFO_t
	{
		//! @class Gray::cTypeInfo
		//! Get type info about a C++ class via typeid(TYPE)
		//! Supplement/helper for built in TYPEINFO_t via typeid()
		//! Similar to MFC CRuntimeClass
		//! @note ASSUME TYPEINFO_t/type_info Always supports name(), hashcode() ?
		//! @note This is NOT natively related to vtable/vfptr at all ! NO _CPPRTTI is required.
		//! @todo get dynamic List of virtual members from vtable?

	public:
		size_t get_HashCode() const noexcept
		{
			// HASHCODE_t
#ifdef _MSC_VER
			return hash_code();
#else
			return (size_t) this;
#endif
		}
		const char* get_Name() const noexcept
		{
			// Get the user friendly version of the name.
			return name();
		}


#ifdef _CPPRTTI
		// vtable/vfptr related stuff.
		static const void** GRAYCALL Get_vtable(void* p);
		static const char* GRAYCALL GetMemberName_TODO(const void** vtable, int i);
#endif

	};
}
#endif
