//
//! @file cUnitTest.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cUnitTest.h"
#include "cDebugAssert.h"
#include "cFilePath.h"
#include "cLogMgr.h"
#include "cArray.h"
#include "cAppState.h"
#include "cAppConsole.h"
#include "cSingletonPtr.h"
#include "cFileDir.h"
#include "cNewPtr.h"
#include "cSystemInfo.h"
#include "cTypes.h"
#include "cTimeDouble.h"
#include "cAppImpl.h"

#if USE_UNITTESTS

#ifdef USE_UNITTESTS_MS
// NOTE: This requires runtime path set to find CppUnitTestFramework.dll or CppUnitTestFramework.x64.dll as well. 
// $(DevEnvDir)\CommonExtensions\Microsoft\TestWindow\Extensions\CppUnitFramework = D:\Programs\Microsoft Visual Studio 12.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\Extensions\CppUnitFramework
#pragma comment(lib,"Microsoft.VisualStudio.TestTools.CppUnitTestFramework.lib")
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#endif

namespace Gray
{
	class GRAYCORE_LINK cUnitTestLogger : public cLogAppender, public cRefBase
	{
		//! @class Gray::cUnitTestLogger
		//! M$ unit tests require linked library CppUnitTestFramework. USE_UNITTESTS_MS

		cFile m_File; // like CLogFileDay

	public:
		IUNKNOWN_DISAMBIG(cRefBase);

		cUnitTestLogger()
		{
			// Assume we compile in the same environment as we unit test.
			cUnitTests::InitLog();
		}

		bool CreateLogFile(const FILECHAR_t* pszFileDir)
		{
			cTimeUnits tStart;
			tStart.InitTimeNow();
			cStringF sLogFileName;
			sLogFileName.Format(_FN("UnitTestResults%04d%02d%02d_") _FN(GRAY_COMPILER_NAME) _FN("_") _FN(GRAY_BUILD_NAME) _FN(MIME_EXT_log),
				tStart.m_wYear, tStart.m_wMonth, tStart.m_wDay);
			HRESULT hRes = m_File.OpenX(cFilePath::CombineFilePathX(pszFileDir, sLogFileName),
				OF_CREATE | OF_SHARE_DENY_NONE | OF_READWRITE | OF_TEXT);
			if (FAILED(hRes))
				return false;
			return true;
		}

		virtual HRESULT WriteString(const LOGCHAR_t* pszMsg) override
		{
			//! Pass messages to M$
#ifdef USE_UNITTESTS_MS
			if (cUnitTestCur::IsInMSTest())
			{
				::Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(pszMsg);
			}
#endif
			if (m_File.isFileOpen())
			{
				m_File.WriteString(pszMsg);
			}
			return 1;
		}
	};

	UNITTEST_LEVEL_TYPE cUnitTestCur::sm_nTestLevel = UNITTEST_LEVEL_Common;	// UNITTEST_LEVEL_Common
	cLogProcessor* cUnitTestCur::sm_pLog = nullptr;	// cLogMgr::I()
	int cUnitTestCur::sm_iFailures = 0;

	cStringF cUnitTestCur::sm_sTestInpDir;
	cStringF cUnitTestCur::sm_sTestOutDir;

	bool cUnitTestCur::sm_bCreatedUnitTests = false;
	int	cUnitTestCur::sm_nCreatedUnitTests = 0;

	const cStrConst cUnitTestCur::k_asTextLines[] =	// sample test data
	{
		CSTRCONST("four"),
		CSTRCONST("one"),
		CSTRCONST("money"),
		CSTRCONST("two"),
		CSTRCONST("red"),
		CSTRCONST("blue"),
		CSTRCONST("get"),
		CSTRCONST("go"),
		CSTRCONST("to"),
		CSTRCONST("now"),
		CSTRCONST("cat"),
		CSTRCONST("green"),
		CSTRCONST("ready"),
		CSTRCONST("three"),
		CSTRCONST("show"),
		CSTRCONST("shoe"),
		CSTRCONST("buckle"),
		CSTRCONST("my"),
		cStrConst(nullptr,nullptr),
	};

	const cStrConst cUnitTestCur::k_sTextBlob = CSTRCONST( // [ cUnitTestCur::k_TEXTBLOB_LEN+1 ]
		"This is a line of test data's\n\n\
\tFour score and seven years ago\n\
Our forefathers brought upon this continent a new nation.\n\
Conceived in liberty and dedicated to the proposition that all\n\
men are created equal. We are now engaged in a great civil war. blah. blah.\n\n\
\tWe the people in order to form a more perfect union\n\
establish justice and ensure domestic tranquility\n\
provide for the common defense\n\
promote the general welfare and\n\
ensure the blessings of liberty, To ourselves and our posterity\n\
do ordain and establish this constitution of the United States of America\n\n");

	const FILECHAR_t* GRAYCALL cUnitTestCur::get_TestOutDir() // static
	{
		//! Get a temporary directory for use by UnitTests
		if (sm_sTestOutDir.IsEmpty())
		{
			sm_sTestOutDir = cAppState::I().GetTempDir(_FN(GRAY_NAMES));
			ASSERT(!sm_sTestOutDir.IsEmpty());
		}
		return sm_sTestOutDir.get_CPtr();
	}

	const FILECHAR_t* GRAYCALL cUnitTestCur::get_TestInpDir() // static
	{
		//! Get source of input files for tests.
		//! e.g. "C:\Dennis\Source\Gray\"

		if (sm_sTestInpDir.IsEmpty())
		{
#ifdef USE_UNITTESTS_MS
			if (IsInMSTest())
			{
				sm_sTestInpDir = cFilePath::GetFilePathUpDir1(cFilePath::GetFileDir(_GT(__FILE__)), k_StrLen_UNK, 2);
			}
			else
#endif
			{
				sm_sTestInpDir = cFilePath::GetFilePathUpDir1(cAppState::get_CurrentDir());

				// Move this path if the sources have moved.
#ifdef __linux__
				// s_sTestInpDir =_FN("/home/Dennis/grayspace/") _FN(GRAY_NAMES)  _FN(FILESTR_DirSep);	// path for test source files.
#else
				// s_sTestInpDir = _FN("c:\\Dennis\\") _FN("Source") _FN(FILESTR_DirSep) _FN(GRAY_NAMES)  _FN(FILESTR_DirSep);	// path for test source files.
#endif
			}
			ASSERT(!sm_sTestInpDir.IsEmpty());
		}
		return sm_sTestInpDir.get_CPtr();
	}

	void GRAYCALL cUnitTestCur::SetTestLevel(UNITTEST_LEVEL_TYPE nTestLevel) // static
	{
		sm_nTestLevel = nTestLevel;
		sm_iFailures = 0;
	}

	bool GRAYCALL cUnitTestCur::IsTestInteractive() noexcept // static
	{
		//! is a user expected to interact with or verify the output of the tests ?
		if (sm_nTestLevel <= UNITTEST_LEVEL_Crit)
			return true;
		if (sm_nTestLevel >= UNITTEST_LEVEL_All)
			return true;
#if 0 // def _DEBUG
		// Is debugger attached?
		if (cAppState::isDebuggerPresent())
			return true;
#endif
		return false;
	}

#if defined(USE_UNITTESTS_MS)
	bool GRAYCALL cUnitTestCur::IsInMSTest() noexcept // static 
	{
		//! Is running as unit test ? Just because it was compiled for M$ Test doesn't mean I am running it that way.
		return !sm_bCreatedUnitTests && sm_nCreatedUnitTests > 0;
	}
#endif

	bool GRAYCALL cUnitTestCur::TestInteractivePrompt(const char* pszMsg) noexcept // static
	{
		//! prompt the user to manually check some output from the test.
		//! require user to press key or button.
		//! @return true = continue.

		if (!IsTestInteractive())
			return false;
		bool bRet = false;
#ifdef _WIN32
		const int iRet = _GTN(::MessageBox)(WINHANDLE_NULL, StrArg<GChar_t>(pszMsg), _GT("Gray cUnitTest"), MB_OKCANCEL);
		bRet = (iRet == IDOK);
#else	// __linux__
		// console.
		cAppConsole& console = cAppConsole::I();
		console.WriteString(pszMsg);
		int iRet = console.ReadKeyWait();
		bRet = (iRet == 'y');
#endif
		// return input from user.
		return bRet;
	}

	//****************************************************************************

	cUnitTest::cUnitTest()
	{
		sm_nCreatedUnitTests++;
#ifdef USE_UNITTESTS_MS
		if (cUnitTestCur::IsInMSTest())
		{
			// add special log appender. Logger. cUnitTestLogger
			// M$ can create cUnitTest outside of the cUnitTests
			cLogMgr& log = cLogMgr::I();
			if (log.FindAppenderType(typeid(cUnitTestLogger), true) == nullptr)
			{
				log.AddAppender(new cUnitTestLogger());	// route logs here.
			}
		}
#endif
	}
#ifdef USE_UNITTESTS_MS
	cUnitTest::~cUnitTest()	noexcept(false)// virtual
#else
	cUnitTest::~cUnitTest()	// virtual
#endif
	{
#ifdef USE_UNITTESTS_MS
		// remove log appender.
		if (cUnitTestCur::IsInMSTest() && sm_nCreatedUnitTests == 1)
		{
			cLogMgr& log = cLogMgr::I();
			if (log.FindAppenderType(typeid(cUnitTestLogger), true) != nullptr)
			{
				cLogMgr::I().RemoveAppenderType(typeid(cUnitTestLogger), true);
				cDebugAssert::sm_pAssertCallback = nullptr;
				// sm_sTestInpDir.Empty();
			}
		}
#endif
		sm_nCreatedUnitTests--;
	}

	//****************************************************************************

	cUnitTestRegister::cUnitTestRegister(const LOGCHAR_t* pszTestName, UNITTEST_LEVEL_TYPE nTestLevel)
		: m_pszTestName(pszTestName)
		, m_nTestLevel(nTestLevel)
	{
		//! May be constructed in 'C' static init code.
		//! register myself in the unit test list.
		cUnitTests& ut = cUnitTests::I();
		ut.m_aUnitTests.Add(this);
	}

	cUnitTestRegister::~cUnitTestRegister() // virtual
	{
		//! When a module is released, all its unit tests MUST be destroyed.
		cUnitTests& ut = cUnitTests::I();
		ut.m_aUnitTests.RemoveArg(this);
	}

	void cUnitTestRegister::UnitTest()
	{
		//! Create the cUnitTest object and run the cUnitTest.
		ASSERT(this != nullptr);
		cNewPtr<cUnitTest> pUnitTest(CreateUnitTest());
		pUnitTest->UnitTest();
	}

	//****************************************************************************

	cUnitTests::cUnitTests()
		: cSingleton<cUnitTests>(this, typeid(cUnitTests))
	{
	}

	bool cUnitTests::RegisterUnitTest(cUnitTestRegister* pTest)
	{
		if (m_aUnitTests.HasArg(pTest))
			return false;
		m_aUnitTests.Add(pTest);
		return true;
	}

	cUnitTestRegister* cUnitTests::FindUnitTest(const char* pszName) const
	{
		//! Find a test by name.
		for (ITERATE_t i = 0; i < m_aUnitTests.GetSize(); i++)
		{
			cUnitTestRegister* pUt = m_aUnitTests[i];
			if (!StrT::Cmp(pUt->m_pszTestName, pszName))
				return pUt;
		}
		return nullptr;
	}

	bool CALLBACK cUnitTests::UnitTest_AssertCallback(const char* pszExp, const cDebugSourceLine& src) // static AssertCallback_t
	{
		//! Assert was called during a unit test.
		//! AssertCallback_t sm_pAssertCallback = cDebugAssert::Assert_Fail was called in UnitTest. GRAYCALL
		cUnitTestCur::sm_iFailures++;
#ifdef USE_UNITTESTS_MS
		__LineInfo lineInfo(cStringW(src.m_pszFile), src.m_pszFunction, src.m_uLine);	// StrArg
		Assert::IsTrue(false, cStringW(pszExp), &lineInfo);
		return false;
#else
		UNREFERENCED_PARAMETER(pszExp);
		UNREFERENCED_REFERENCE(src);
		return true;	// fall through to log and continue.	NOT cDebugAssert::Assert_System()
#endif
	}

	bool cUnitTests::TestTypes()
	{
		//! Check some basic compiler assumptions.
		//! Called early in unit tests
		//! _MSC_VER, 32 bit, sizeof(bool)=1,sizeof(enum)=4,sizeof(int)=4,sizeof(long)=4
		//! _MSC_VER, 64 bit, sizeof(bool)=1,sizeof(enum)=4,sizeof(int)=4,sizeof(long)=4,
		//! __GNUC__, 32 bit, sizeof(int)=4, sizeof(long)=?
		//! __GNUC__, 64 bit, sizeof(int)=4, sizeof(long)=8

		// STATIC_ASSERT(sizeof(void) == 0, void);
		STATIC_ASSERT(sizeof(char) == 1, char);
		STATIC_ASSERT(sizeof(short) == 2, short);
		STATIC_ASSERT(sizeof(INT32) == 4, INT32);
		STATIC_ASSERT(sizeof(UINT32) == 4, UINT32);
		STATIC_ASSERT(sizeof(CUnion32) == 4, CUnion32);	// 32 bits exactly.
		STATIC_ASSERT(sizeof(INT64) == 8, INT64);
		STATIC_ASSERT(sizeof(UINT64) == 8, UINT64);
		STATIC_ASSERT(sizeof(CUnion64) == 8, CUnion64);	// 64 bits exactly.
		STATIC_ASSERT(sizeof(DWORD) == 4, DWORD);			// _MSC_VER 64=4, _MSC_VER 32=4

		size_t iSizeBool = sizeof(bool);	// _MSC_VER 32=1, _MSC_VER 64=1
		UNITTEST_TRUE(iSizeBool == 1);
		size_t iSizeEnum = sizeof(enum UNITTEST_LEVEL_TYPE);	// __MSC_VER 32=4, _MSC_VER 64=4
		UNITTEST_TRUE(iSizeEnum == 4);
		size_t nSizeWChar = sizeof(wchar_t);	// _MSC_VER 64=2, __MSC_VER 32=2
#ifdef __GNUC__
		UNITTEST_TRUE(nSizeWChar == 4);
#else
		UNITTEST_TRUE(nSizeWChar == 2);
#endif

		size_t iSizeFloat = sizeof(float);	// __MSC_VER 32=4, _MSC_VER 64=4
		UNITTEST_TRUE(iSizeFloat == 4);
		size_t iSizeDouble = sizeof(double);	// __MSC_VER 32=8, _MSC_VER 64=8
		UNITTEST_TRUE(iSizeDouble == 8);	// 64 bits

#ifdef USE_LONG_DOUBLE
	// long double = 10 byte, 80 bit float point ? ieee854_float80_t
		UNITTEST_TRUE(sizeof(long double) > 8);
#endif

#ifdef _WIN32
		STATIC_ASSERT(true == TRUE, TRUE);	// _WIN32 only
		bool bVal = true;
		UNITTEST_TRUE(((BYTE)bVal) == 1);
		bVal = false;
		UNITTEST_TRUE(((BYTE)bVal) == 0);

		size_t nSizeUINT_PTR = sizeof(UINT_PTR);	// _WIN32=4 _WIN64=8
		UNITTEST_TRUE(nSizeUINT_PTR >= 4);
		size_t nSizeATOM = sizeof(ATOM);			// _WIN32=2 _WIN64=2
		UNITTEST_TRUE(nSizeATOM == 2);
		size_t nSizeHWND = sizeof(HWND);			// _WIN32=4 _WIN64=8
		UNITTEST_TRUE(nSizeHWND >= 4);
		size_t nSizeHCURSOR = sizeof(HCURSOR);		// _WIN32=4 _WIN64=8
		UNITTEST_TRUE(nSizeHCURSOR >= 4);
		size_t nSizePOINT = sizeof(POINT);			// _WIN32=8 _WIN64=8
		UNITTEST_TRUE(nSizePOINT == 8);
		size_t nSizeMSG = sizeof(MSG);				// _WIN32=28 _WIN64=48
		UNITTEST_TRUE(nSizeMSG >= 2);
#endif

		// Test the ambiguous/questionable types first.
		size_t iSizeInt = sizeof(int);		// _MSC_VER 64=4, and _MSC_VER 32=4
		UNITTEST_TRUE(iSizeInt == 4);

		// USE_LONG_AS_INT64 // 4 or 8 ?
		size_t iSizeLong = sizeof(long);		// _MSC_VER 64=4 _MSC_VER 32=4, __GNUC__ 64=?
		UNITTEST_TRUE(iSizeLong >= 4);
		UNITTEST_TRUE(iSizeLong == _SIZEOF_LONG);
		size_t iSizeLongInt = sizeof(long int);		// _MSC_VER 64=4, _MSC_VER 32=4
		UNITTEST_TRUE(iSizeLong == iSizeLongInt);

		size_t nSizeSize = sizeof(size_t);		// _MSC_VER 64=8 _MSC_VER 32=4
		UNITTEST_TRUE(nSizeSize >= 4);

		StrLen_t nLenStrA = StrT::Len<char>(k_sTextBlob);
		StrLen_t nLenStrW = StrT::Len<wchar_t>(k_sTextBlob);
		UNITTEST_TRUE(nLenStrW == nLenStrA);
		UNITTEST_TRUE(nLenStrA == k_TEXTBLOB_LEN);

#define k_abc "abcdefghijkl"
		UNITTEST_TRUE(sizeof(k_abc) == 13);	// confirm presumed behavior
		UNITTEST_TRUE(_countof(k_abc) == 13);	// confirm presumed behavior
		UNITTEST_TRUE(STRMAX(k_abc) == 12);
#undef k_abc

		// is endian set correctly ?
		CUnion32 u32;
		u32.u_dw = 0x12345678UL;
#ifdef USE_LITTLE_ENDIAN
		UNITTEST_TRUE(u32.u_b[0] == 0x78);
#else
		UNITTEST_TRUE(u32.u_b[0] == 0x12);
#endif

#ifndef __GNUC__
		size_t iOffset = offsetof(cException, m_pszDescription);	// NOTE: Always has a warning using __GNUC__
		UNITTEST_TRUE(iOffset > 8);	// check offsetof().
#endif

	// Test some size assumptions. cPtrFacade
		cNewPtr<int> pNewObj;
		UNITTEST_TRUE(sizeof(cNewPtr<int>) == sizeof(int*));
		cRefBasePtr pRefObj;
		UNITTEST_TRUE(sizeof(cRefBasePtr) == sizeof(cRefBase*));
		cIUnkBasePtr pIRefObj;
		// UNITTEST_TRUE( sizeof(cIUnkBasePtr) == sizeof(IUnknown*));

		// Test CHECKPTR_CAST(TYPE2, get_Single());
		cAppState* pTest1 = cAppState::get_Single();
		cAppState* pTest2 = CHECKPTR_CAST(cAppState, pTest1);
		UNITTEST_TRUE(pTest1 == pTest2);

		return true;
	}

	void GRAYCALL cUnitTests::InitLog() // static
	{
		// Attach logger.
 		sm_pLog = cLogMgr::get_Single();  // route logs here.
		cDebugAssert::sm_pAssertCallback = UnitTest_AssertCallback;		// route asserts back here.
	}

	HRESULT cUnitTests::UnitTests(UNITTEST_LEVEL_TYPE nTestLevel, const LOGCHAR_t* pszTestNameMatch)
	{
		//! Execute all the registered unit tests at the selected level.
		//! @note assume each test is responsible for its own resources.
		//! @arg pszTestNameMatch = a name wildcard. possibly in a comma separated list. (or nullptr)

		SetTestLevel(nTestLevel);

		if (sm_nTestLevel <= UNITTEST_LEVEL_None)	// tests turned off at run time.
			return S_OK;

		sm_bCreatedUnitTests = true;

		// Where to send the test output?
		InitLog();	// route logs here.
		 
		cTimePerf::InitFreq();

		cTimeSys tStart(cTimeSys::GetTimeNow());

		// Make sure get_CurrentDir() is set correctly

		cStringF sTestInpDir = get_TestInpDir();
		cStringF sTestOutDir = get_TestOutDir();

		// Create a results log file with the date stamped on it + Build config. x86 vs x64 etc.
		cRefPtr<cUnitTestLogger> pLogFile(new cUnitTestLogger);
		if (pLogFile->CreateLogFile(sTestOutDir))
		{
			cLogMgr::I().AddAppender(pLogFile);
		}

		sm_pLog->addDebugInfoF("cUnitTests input from '%s'", LOGSTR(sTestInpDir));
		sm_pLog->addDebugInfoF("cUnitTests output to '%s'", LOGSTR(sTestOutDir));

		cArrayString<LOGCHAR_t> aNames;
		if (pszTestNameMatch == nullptr)
		{
			sm_pLog->addDebugInfoF("cUnitTests STARTING %d TESTS at level %d", m_aUnitTests.GetSize(), nTestLevel);
		}
		else
		{
			sm_pLog->addDebugInfoF("cUnitTests STARTING '%s' from %d TESTS", LOGSTR(pszTestNameMatch), m_aUnitTests.GetSize());
			aNames.SetStrSep(pszTestNameMatch, ',');
		}

		// Display build/compile info. date, compiler, _MFC_VER/_AFXDLL, 64/32 bit.
		sm_pLog->addDebugInfoF("Build: '%s' v%d for '%s' on '%s'", GRAY_COMPILER_NAME, GRAY_COMPILER_VER, GRAY_BUILD_NAME, __DATE__);
		// Display current run environment info. OS type, 64/32 bit, OS version, CPU's.
		sm_pLog->addDebugInfoF("OS: '%s'", LOGSTR(cSystemInfo::I().get_OSName()));

		// Test presumed behavior of compiler types.
		TestTypes();

		// Run all s_aUnitTests registered tests at sm_nTestLevel
		char szDashes[64];
		cValArray::FillSize<BYTE>(szDashes, STRMAX(szDashes), '-');
		szDashes[STRMAX(szDashes)] = '\0';
		UNITTEST_TRUE(cMem::IsValid(szDashes, sizeof(szDashes)));

		cLogAppendDebug::AddAppenderCheck(nullptr);	// push log to OutputDebugString()

#if defined(_MSC_VER) && defined(_DEBUG) && ! defined(UNDER_CE)
		cHeap::Init(_CRTDBG_ALLOC_MEM_DF);
#endif

		ITERATE_t iTestsRun = 0;
		for (ITERATE_t i = 0; i < m_aUnitTests.GetSize(); i++)
		{
			cUnitTestRegister* pUnitTest = m_aUnitTests[i];
			if (pszTestNameMatch == nullptr)
			{
				if (pUnitTest->m_nTestLevel > sm_nTestLevel)
					continue;
			}
			else
			{
				int j = 0;
				for (; j < aNames.GetSize(); j++)
				{
					if (StrT::MatchRegEx<LOGCHAR_t>(pUnitTest->m_pszTestName, aNames[j], true) > 0)
						break;
				}
				if (j >= aNames.GetSize())	// NOT matched.
					continue;
				aNames.RemoveAt(j);	// Did this.
			}

			StrLen_t iLenName = StrT::Len(pUnitTest->m_pszTestName) + 1;
			UNITTEST_TRUE(iLenName > 0 && iLenName < STRMAX(szDashes));
			szDashes[_countof(szDashes) - iLenName] = '\0';

			sm_pLog->addDebugInfoF("cUnitTest '%s' run  %s", LOGSTR(pUnitTest->m_pszTestName), szDashes);
			szDashes[_countof(szDashes) - iLenName] = '-';

			cTimePerf tPerfStart(true);		// Time a single test for performance changes.

			pUnitTest->UnitTest();

			cTimePerf tPerfStop(true);		// Time a single test for performance changes.

			if (sm_iFailures > 0)
			{
				// Assume cDebugAssert::Assert_Fail was also called.
				sm_pLog->addDebugInfoF("cUnitTest FAILED '%s' Test %d", LOGSTR(pUnitTest->m_pszTestName), iTestsRun);
				return E_FAIL;
			}

			iTestsRun++;

			// Do a heap integrity test between unit tests. just in case.
			bool bHeapCheck = cHeap::Check();
			if (!bHeapCheck && !::Gray::cDebugAssert::Assert_Fail("cHeap::Check", DEBUGSOURCELINE))
			{
				// Assume cDebugAssert::Assert_Fail was also called.
				sm_pLog->addDebugInfoF("cUnitTest FAILED '%s' Heap Corruption.", LOGSTR(pUnitTest->m_pszTestName));
				return E_FAIL;
			}

			// How long did it take?
			ASSERT(tPerfStop.get_Perf() >= tPerfStart.get_Perf());
			TIMEPERF_t iPerfDiff = tPerfStop.get_Perf() - tPerfStart.get_Perf();
			double dDaysDiff = cTimePerf::ToDays(iPerfDiff);
			ASSERT(dDaysDiff >= 0 && dDaysDiff < 1);
			cString sTimeSpan = cTimeDouble::GetTimeSpanStr(dDaysDiff);
			StrLen_t nLenText = sTimeSpan.GetLength() + iLenName + 8;
			if (nLenText > STRMAX(szDashes))
				nLenText = STRMAX(szDashes);
			szDashes[_countof(szDashes) - nLenText] = '\0';
			sm_pLog->addDebugInfoF("cUnitTest '%s' complete in %s %s", LOGSTR(pUnitTest->m_pszTestName), LOGSTR(sTimeSpan), szDashes);
			szDashes[_countof(szDashes) - nLenText] = '-';
		}

		sm_pLog->addDebugInfoF("cUnitTests ENDING %d/%d TESTS in %s", iTestsRun, m_aUnitTests.GetSize(),
			LOGSTR(cTimeInt::GetTimeSpanStr(tStart.get_AgeSec())));

		if (pszTestNameMatch != nullptr && aNames.GetSize() > 0)
		{
			// Didn't find some names !
			sm_pLog->addDebugInfoF("cUnitTest FAILED to find test for '%s'", LOGSTR(aNames[0]));
		}


#ifdef USE_IUNK_TRACE
		cPtrTrace::TraceDump(*sm_pLog, 0);
#endif

		// Clear the temporary directory ? get_TestOutDir()

		return (HRESULT)iTestsRun;
	}
}

#endif	// USE_UNITTESTS
