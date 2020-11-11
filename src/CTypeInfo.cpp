//
//! @file cTypeInfo.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cTypeInfo.h"
#include "cTypes.h"

namespace Gray
{

#ifndef _MSC_VER	// __GNUC__
#define CTYPE_DEF(a,_TYPE,c,d,e,f,g,h)  template<> const _TYPE CTypeLimit<_TYPE>::k_Min = e; template<> const _TYPE CTypeLimit<_TYPE>::k_Max = f; 
#include "cTypes.tbl"
#undef CTYPE_DEF
#endif

	const char* cTypeInfo::GetMemberName(int i) const
	{
		//! read the vptr/vtable to get a list of names of the virtual exposed members.
		//! @todo Get List of members.
		//! @return nullptr = end of list.
		if (i < 0)
			return nullptr;
		if (i > 1)
			return nullptr;

		// _MSC_VER
		// __GNUC__

		return "test";
	}
}

//*************************************************************

#if USE_UNITTESTS
#include "cUnitTest.h"
#include "cLogMgr.h"

DECLARE_INTERFACE(ITestClass0)
{
	IGNORE_WARN_INTERFACE(ITestClass0);
	virtual void f1() = 0;
};

UNITTEST_CLASS(cTypeInfo)
{
	class cTestClass1 : public ITestClass0
	{
		virtual void f1() = 0;
	};
	class cTestClass2 : public ITestClass0
	{
		virtual void f1() = 0;
		virtual void f2() = 0;
	};
	class cTestClass3 : public cTestClass2, public cTestClass1
	{
		virtual void f1()
		{
		}
		virtual void f2()
		{
		}
		virtual void f3()
		{
		}
	};

	UNITTEST_METHOD(cTypeInfo)
	{
		// Enumerate methods from the vtable. __vfptr

		int nInt = 0;
		const TYPEINFO_t& tInt0 = typeid(int);
		const TYPEINFO_t& tInt1 = typeid(nInt);
		// const TYPEINFO_t& tInt2 = typeid( decltype(nInt));		// Test use of C++11 feature "-Wc++0x-compat"
		UNITTEST_TRUE(tInt0 == tInt1);
		// UNITTEST_TRUE(tInt0 == tInt2);
		UNITTEST_TRUE(nInt == 0);	// just for a ref.

		const cTypeInfo& tSmart0 = (const cTypeInfo&) typeid(cRefBase);
		cRefBase oSmart;
		const cTypeInfo& tSmart1 = (const cTypeInfo&) typeid(oSmart);

		size_t h0 = tSmart0.get_HashCode();
		size_t h1 = tSmart1.get_HashCode();
		UNITTEST_TRUE(h1 != 0);
		UNITTEST_TRUE(h0 == h1);

		cTestClass3 oTC3;
		const cTypeInfo& tTC3 = (const cTypeInfo&) typeid(oTC3);
		const cTypeInfo& tTC1 = (const cTypeInfo&) typeid(cTestClass1);
		const cTypeInfo& tTC2 = (const cTypeInfo&) typeid(cTestClass2);
		UNITTEST_TRUE(h0 != tTC3.get_HashCode());

#ifdef _MSC_VER
		const char* pszRawName1 = tSmart1.raw_name();	// Get M$ Mangled/Raw name ".?AVcRefBase@Gray@@"
		UNITTEST_TRUE(pszRawName1 != nullptr);
#endif

		const char* pszName1 = tSmart1.get_Name();		// "class Gray::cRefBase"
		UNITTEST_TRUE(pszName1 != nullptr);

		const char* pszName2 = tTC3.get_Name();		// "class cUnitTest_cTypeInfo::cTestClass3"
		UNITTEST_TRUE(pszName2 != nullptr);

		const cTypeInfo* ptTC1 = &tTC1;
		UNITTEST_TRUE(ptTC1 != nullptr);
		const cTypeInfo* ptTC2 = &tTC2;
		UNITTEST_TRUE(ptTC2 != nullptr);

		cTestClass3* pTC3 = &oTC3;
		UNITTEST_TRUE(pTC3 != nullptr);
		cTestClass1* pTC1 = &oTC3;
		cTestClass2* pTC2 = &oTC3;
		UNITTEST_TRUE( (void*) pTC1 != (void*) pTC2);
		cTestClass1* pTC1b = DYNPTR_CAST(cTestClass1,pTC2);	// convert cTestClass2 back to peer cTestClass1. __RTDynamicCast
		UNITTEST_TRUE(pTC1 == pTC1b);

		// Enumerate base classes and virtual methods from the vtable.
		// void* pVTable = pTC3->_vptr;
	}
};
UNITTEST_REGISTER(cTypeInfo, UNITTEST_LEVEL_Core);
#endif
