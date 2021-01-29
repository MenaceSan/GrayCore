//
//! @file cUnitTest.h
//! Included from c++ file to implement unit test. Compatible with M$ unit tests.  
//! @note Don't include this from some other header file. Only use in implementation of a test.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#ifndef _INC_cUnitTest_H
#define _INC_cUnitTest_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cUnitTestDecl.h"
#include "StrConst.h"
#include "StrArg.h"
#include "FileName.h"
#include "cLogLevel.h"
#include "cLogMgr.h"
#include "cDebugAssert.h"
#include "cSingleton.h"
#include "cArray.h"
#include "cFilePath.h"
#include "cAppState.h"
#include "cOSModImpl.h"

namespace Gray
{
	class cLogProcessor;

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

	class cUnitTestRegister;

	struct GRAYCORE_LINK cUnitTestCur	// static
	{
		//! @struct Gray::cUnitTestCur
		//! static struct for context information about the CURRENT running test. singleton
		//! Base class for all derived tests cUnitTest
		//! Assume we compile in the same environment as we unit test.

		static int sm_nCreatedUnitTests;		//!< Count the cUnitTest objects I have created. NOT just m_aUnitTests
		static const FILECHAR_t* k_TestFiles;	// a sub directory under m_sTestOutDir containing all the test files.

		// Sample Test const data.
		static const StrLen_t k_TEXTBLOB_LEN = 566;	//!< StrT::Len(k_sTextBlob) = 0x236
		static const cStrConst k_sTextBlob;			//!< a single allocated k_TEXTBLOB_LEN+1
		static const StrLen_t k_TEXTLINES_QTY = 18;	//!< STRMAX= _countof(k_asTextLines)-1
		static const cStrConst k_asTextLines[k_TEXTLINES_QTY + 1];		//!< nullptr terminated array of lines of text.

		static bool GRAYCALL TestTypes();
	};

	class GRAYCORE_LINK cUnitTest : public cUnitTestCur
	{
		//! @class Gray::cUnitTest
		//! a unit test for a specific type of thing. attached to a cOSModImpl
		//! Similar to something like: JUnit/CppUnit, Boost.Test or CxxTest.
		//! Similar to "::Microsoft::VisualStudio::CppUnitTestFramework::TestClass<className>"
		//! @note All unit tests should be allowed to run in something like release mode! or be compiled out.

	public:
		cUnitTest();
		virtual ~cUnitTest() noexcept(false);	// M$ test force use of noexcept(false); 

		const FILECHAR_t* get_TestInpDir() const;

		//! Run the test.
		virtual void RunUnitTest() = 0;	 
	};

	class GRAYCORE_LINK cUnitTestRegister
	{
		//! @class Gray::cUnitTestRegister
		//! Hold the registration for a type of cUnitTest.
		//! ALWAYS constructed in 'C' static init code. The cUnitTest itself is constructed on run demand.
		//! Assume static init is NOT multi threaded so no thread locking is required.
	public:
		const LOGCHAR_t* m_pszTestName;			//!< Display Name for the unit test.
		const UNITTEST_LEVEL_TYPE m_nTestLevel;	//!< at what level does this test run?
	protected:
		cUnitTestRegister(const LOGCHAR_t* pszTestName, UNITTEST_LEVEL_TYPE nTestLevel = UNITTEST_LEVEL_Core);
		virtual ~cUnitTestRegister();
		virtual cUnitTest* CreateUnitTest() = 0;	//!< must implement this.
	public:
		void RunUnitTest();	//!< Create and run the unit test cUnitTest.
	};

	template <class T = cUnitTest>
	class cUnitTestRegisterT : public cUnitTestRegister, public cSingletonStatic< cUnitTestRegisterT<T> >
	{
		//! @class Gray::cUnitTestRegisterT
		//! a singleton to register a unit test for a specific type of thing. Allow creation of its cUnitTest based implementation class.
		//! ALWAYS constructed in 'C' static init code. cSingletonStatic< cUnitTestRegisterT<T> >
		//! Assume static init is NOT multi threaded so no thread locking is required.
	public:
		cUnitTestRegisterT(const LOGCHAR_t* pszTestName, UNITTEST_LEVEL_TYPE nTestLevel = UNITTEST_LEVEL_Core)
			: cUnitTestRegister(pszTestName, nTestLevel)
			, cSingletonStatic< cUnitTestRegisterT<T> >(this)
		{
		}
		virtual cUnitTest* CreateUnitTest() override
		{
			//! create derived version of cUnitTest
			//! Never create pure virtual cUnitTest directly of course.
			return new T();
		}
	};

	class GRAYCORE_LINK cUnitTestAppState
	{
		//! @class Gray::cUnitTestAppState
		//! set to APPSTATE_Run for testing purposes.
		//! Unit Tests might use this reentrant with(or without) cAppStateMain ?
		//! e.g. cUnitTestAppState inmain;
	private:
		cAppState& m_AppState;				//!< Fast access to this.
		APPSTATE_TYPE_ m_eAppStatePrev;		//!< Restore the true state of the app if we need to.
		THREADID_t m_nMainThreadPrev;		//!< The thread we started with. main().

	public:
		cUnitTestAppState()
			: m_AppState(cAppState::I())
		{
			// called for M$ tests.
			m_eAppStatePrev = m_AppState.get_AppState();
			m_nMainThreadPrev = m_AppState.get_MainThreadId();
			m_AppState.InitAppState();	// set to APPSTATE_Run
		}

		~cUnitTestAppState() noexcept
		{
			m_AppState.put_AppState(m_eAppStatePrev);	// destructors should be called next.
			m_AppState.m_nMainThreadId = m_nMainThreadPrev;
		}
	};

	class GRAYCORE_LINK cUnitTests : public cSingleton<cUnitTests>, public cUnitTestCur
	{
		//! @class Gray::cUnitTests
		//! Singleton class to hold the list of all unit tests registered.
		//! MUST use cSingleton and not cSingletonStatic to prevent C runtime load order problems.

		friend class cUnitTestRegister;

	public:
		cArrayPtr<cUnitTestRegister> m_aUnitTests;	//!< list of all registered unit tests. Register as they get instantiate by C runtime static loader.
		static AssertCallback_t UnitTest_AssertCallback;	//!< redirect assert here for test failure. requires _DEBUG or _DEBUG_FAST.
		AssertCallback_t* m_pAssertOrig;			//! restore the original assert.
		
		UNITTEST_LEVEL_TYPE m_nTestLevel;		//!< The current global test level for UnitTests(). throttle tests at run time.
		cArrayString<LOGCHAR_t> m_aTestNames;	// just run these tests.

		cStringF m_sTestInpDir;				//!< root for source of test input files. might change based on cOSModImpl?
		cStringF m_sTestOutDir;				//!< global config for input files.

		cLogProcessor* m_pLog;			//!< cLogMgr::I() for output of tests.	Why not just use DEBUG_MSG ??

		bool m_bRunning;				//!< We are actively running in the Gray test framework. Not in M$ framework.
		int m_iFailures;				//!< Count total unit test failures.

	public:
		cUnitTests();

		HRESULT InitTestOutDir();
		bool RegisterUnitTest(cUnitTestRegister* pTest);

		void SetTestLevel(UNITTEST_LEVEL_TYPE nTestLevel);
		bool TestActive(const cUnitTestRegister* pUnitTest, bool remove);

		cUnitTestRegister* FindUnitTest(const char* pszName) const;

		bool IsTestInteractive() const noexcept;
		bool TestInteractivePrompt(const char* pszMsg) noexcept;

		const FILECHAR_t* get_TestInpDir() const;
		const FILECHAR_t* get_TestOutDir() const;

		void RunInitialize();
		void RunCleanup();

		//! Run all tests <= this UNITTEST_LEVEL_TYPE
		HRESULT RunUnitTests(UNITTEST_LEVEL_TYPE nTestLevel = UNITTEST_LEVEL_Common, const LOGCHAR_t* pszTestNameMatch = nullptr);

		CHEAPOBJECT_IMPL;	// dynamic singleton
	};

#define UNITTEST_TRUE(x)		ASSERT(x)	// UNITTEST_TRUE is different from a normal ASSERT ?
#define UNITTEST_TRUE2(x,d)		ASSERT(x)	// UNITTEST_TRUE with a description

	// Deprecate all below in favor of UNITTEST2_*
 
#define UNITTEST_REGISTER_NAME(n)	g_UnitTest_##n
#define UNITTEST_REGISTER(n,l)		::Gray::cUnitTestRegisterT< UNITTEST_N(n) > UNITTEST_REGISTER_NAME(n)( #n, l );	// instantiate to register cUnitTest  .

	// Allow an external hard link. optional.
#define UNITTEST_EXT_NAME(n)		g_pUnitTest_##n		//!< a base pointer to cUnitTestRegister for UNITTEST_N(n)
#define UNITTEST_EXT_DEF(n)			cUnitTestRegister* UNITTEST_EXT_NAME(n) = &UNITTEST_REGISTER_NAME(n);

} 	// namespace

using namespace Gray;	// Since this header is typically only included right before the unit test.

#endif	// _INC_cUnitTest_H
