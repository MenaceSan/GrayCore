//
//! @file CUnitTest.h
//! Included from c++ file to implement unit test. Compatible with M$ unit tests.
//! @note Don't include this from some other header file.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_CUnitTest_H
#define _INC_CUnitTest_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "CUnitTestDecl.h"
#include "StrConst.h"
#include "StrArg.h"
#include "FileName.h"
#include "CLogLevel.h"
#include "CLogMgr.h"
#include "CDebugAssert.h"
#include "CSingleton.h"
#include "CArray.h"
#include "CFilePath.h"
#include "CAppState.h"
#include "COSModImpl.h"

#ifdef USE_UNITTESTS
#if ! defined(USE_64BIT) && ! defined(_MFC_VER) && defined(_MSC_VER) && _MSC_VER >= 1800
// Only seems to work in 32 bit mode.
#define USE_UNITTESTS_MS	// Use the native M$ unit tests that integrate into VS13. requires inclusion of .lib and Microsoft.VisualStudio.TestTools.CppUnitTestFramework.dll
#endif
#if defined(USE_UNITTESTS_MS)
#include <../UnitTest/Include/CppUnitTest.h>	// in System includes.
#endif
#endif

namespace Gray
{
	class CLogProcessor;

	enum UNITTEST_LEVEL_TYPE
	{
		//! @enum Gray::UNITTEST_LEVEL_TYPE
		//! Which level/type of unit tests should we run ?
		UNITTEST_LEVEL_None = 0,
		UNITTEST_LEVEL_Crit,	//!< 1=critical tests. usually stuff i want to debug now.
		UNITTEST_LEVEL_Core,	//!< 2=only the most basic tests.
		UNITTEST_LEVEL_Lib,		//!< 3
		UNITTEST_LEVEL_Common,	//!< 4 = Common or application level tests.
		UNITTEST_LEVEL_Slow,	//!< 5=slow tests
		UNITTEST_LEVEL_All,		//!< 6=interactive tests, need special external rigs, db, etc
		UNITTEST_LEVEL_Off,		//!< These test don't work yet. or is broken.
	};

	class CUnitTestRegister;

	struct GRAYCORE_LINK CUnitTestCur	// static
	{
		//! @struct Gray::CUnitTestCur
		//! static struct for context information about the current running test.

		static bool GRAYCALL IsTestInteractive() noexcept;
		static bool GRAYCALL TestInteractivePrompt(const char* pszMsg) noexcept;

#if defined(USE_UNITTESTS_MS)
		static bool GRAYCALL IsInMSTest() noexcept;
#endif

		static const FILECHAR_t* GRAYCALL get_TestOutDir();
		static const FILECHAR_t* GRAYCALL get_TestInpDir();

		static CStringF sm_sTestInpDir;				//!< root for source of test input files.
		static CStringF sm_sTestOutDir;				//!< global config for input files.

		static UNITTEST_LEVEL_TYPE sm_nTestLevel;		//!< The current global test level for UnitTests(). throttle tests at run time.
		static CLogProcessor* sm_pLog;					//!< CLogMgr::I() for output of tests.
		static int sm_iFailures;						//!< Count unit test failures.

		static bool sm_bCreatedUnitTests;		//!< I created the CUnitTests manager singleton.
		static int sm_nCreatedUnitTests;		//!< Count the unit tests I have created.

		static void GRAYCALL SetTestLevel(UNITTEST_LEVEL_TYPE nTestLevel);

		// Sample Test const data.
		static const StrLen_t k_TEXTBLOB_LEN = 566;	//!< StrT::Len(k_sTextBlob) = 0x236
		static const CStrConst k_sTextBlob;			//!< a single allocated k_TEXTBLOB_LEN+1
		static const StrLen_t k_TEXTLINES_QTY = 18;	//!< STRMAX= _countof(k_asTextLines)-1
		static const CStrConst k_asTextLines[];		//!< nullptr terminated array of lines of text.

	};

	class GRAYCORE_LINK CUnitTest : public CUnitTestCur
	{
		//! @class Gray::CUnitTest
		//! a unit test for a specific type of thing.
		//! Similar to something like: JUnit/CppUnit, Boost.Test or CxxTest.
		//! Similar to "::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<className>"
		//! Assume static init is NOT multi threaded so no thread locking is required.
		//! @note All unit tests should be allowed to run in something like release mode! or be compiled out.

	public:
		CUnitTest();

#ifdef USE_UNITTESTS_MS		// register with M$ unit test framework as well. define in global namespace.
		virtual ~CUnitTest() noexcept(false); // Needed for vs2017, vs2019
#else
		virtual ~CUnitTest();
#endif

		//! Run the test.
		virtual void UnitTest() = 0;	// UNITTEST_METHOD()
	};

	class GRAYCORE_LINK CUnitTestRegister
	{
		//! @class Gray::CUnitTestRegister
		//! Hold the registration for a type of CUnitTest.
		//! May be constructed in 'C' static init code. The CUnitTest itself is constructed on run demand.
	public:
		const LOGCHAR_t* m_pszTestName;			//!< Display Name for the unit test.
		const UNITTEST_LEVEL_TYPE m_nTestLevel;	//!< at what level does this test run?
	protected:
		CUnitTestRegister(const LOGCHAR_t* pszTestName, UNITTEST_LEVEL_TYPE nTestLevel = UNITTEST_LEVEL_Core);
		virtual ~CUnitTestRegister();
		virtual CUnitTest* CreateUnitTest() = 0;	//!< must implement this.
		// { return nullptr; }
	public:
		void UnitTest();	//!< Create and run the unit test.
	};

	class GRAYCORE_LINK CUnitTestAppState
	{
		//! @class Gray::CUnitTestAppState
		//! Unit Tests might use this reentrant with(or without) CAppStateMain ?
		//! e.g. CUnitTestAppState inmain;
	private:
		CAppState& m_AppState;				//!< Fast access to this.
		APPSTATE_TYPE_ m_eAppStatePrev;		//!< Restore the true state of the app if we need to.

	public:
		CUnitTestAppState()
			: m_AppState(CAppState::I())
		{
			// called in UNITTEST_METHOD for M$ tests.
			m_eAppStatePrev = m_AppState.get_AppState();
			if (m_eAppStatePrev == APPSTATE_Init || m_eAppStatePrev == APPSTATE_Exit)
			{
				m_eAppStatePrev = APPSTATE_Exit;
				m_AppState.InitAppState();	// set to APPSTATE_Run
			}
			else
			{
				m_AppState.put_AppState(APPSTATE_Run);
			}
		}
		~CUnitTestAppState() noexcept
		{
			m_AppState.put_AppState(m_eAppStatePrev);	// destructors should be called next.
		}
	};

	template <class T = CUnitTest>
	class CUnitTestRegisterT : public CUnitTestRegister
	{
		//! @class Gray::CUnitTestHolder
		//! register a unit test for a specific type of thing. Allow creation of its CUnitTest based implementation class.
		//! May be constructed in 'C' static init code.
	public:
		CUnitTestRegisterT(const LOGCHAR_t* pszTestName, UNITTEST_LEVEL_TYPE nTestLevel = UNITTEST_LEVEL_Core)
			: CUnitTestRegister(pszTestName, nTestLevel)
		{
		}
		virtual CUnitTest* CreateUnitTest() override
		{
			//! Never create pure virtual CUnitTest directly of course.
			return new T();
		}
	};

	class GRAYCORE_LINK CUnitTests : public CSingleton<CUnitTests>, public CUnitTestCur
	{
		//! @class Gray::CUnitTests
		//! Singleton class to hold the list of all unit tests registered.
		//! MUST use CSingleton and not CSingletonStatic to prevent C runtime load order problems.

		friend class CUnitTestRegister;

	public:
		CArrayPtr<CUnitTestRegister> m_aUnitTests;	//!< list of all registered unit tests. Register as they get instantiate by C runtime static loader.
		static AssertCallback_t UnitTest_AssertCallback;	//!< redirect assert here for test failure. requires _DEBUG or _DEBUG_FAST.

	public:
		CUnitTests();

		bool RegisterUnitTest(CUnitTestRegister* pTest);
		CUnitTestRegister* FindUnitTest(const char* pszName) const;
		static void GRAYCALL InitLog();

		//! Run all tests <= this UNITTEST_LEVEL_TYPE
		bool TestTypes();
		HRESULT UnitTests(UNITTEST_LEVEL_TYPE nTestLevel = UNITTEST_LEVEL_Common, const LOGCHAR_t* pszTestNameMatch = nullptr);

		CHEAPOBJECT_IMPL;
	};

#ifdef USE_UNITTESTS_MS		// register with M$ unit test framework as well. define in global namespace.
#define UNITTEST_CLASS(n)		TEST_CLASS( UNITTEST_N(n)) , public CUnitTest
#define UNITTEST_METHOD(x)		public: TEST_METHOD(Test##x) { this->UnitTest(); } virtual void UnitTest() override
#else
#define UNITTEST_CLASS(n)		class UNITTEST_N(n) : public CUnitTest //!< define and implement class. TEST_CLASS(n)
#define UNITTEST_METHOD(x)		public: virtual void UnitTest() override				// call the public virtual as a test. TEST_METHOD(x)
#endif

#define UNITTEST_REGISTER_NAME(n) g_CUnitTest_##n
#define UNITTEST_REGISTER(n,l)	::Gray::CUnitTestRegisterT< UNITTEST_N(n) > UNITTEST_REGISTER_NAME(n)( #n, l );	// instantiate to register UNITTEST_CLASS.

	// Allow an external hard link. optional.
#define UNITTEST_REGISTER_EXT(n)	CUnitTestRegister* UNITTEST_EXT(n) = &UNITTEST_REGISTER_NAME(n);

#define UNITTEST_TRUE(x)		ASSERT(x)	// UNITTEST_TRUE is different form a normal ASSERT
#define UNITTEST_TRUE2(x,d)		ASSERT(x)	// UNITTEST_TRUE with a description

};	// namespace

using namespace Gray;

#endif
