//! @file cUnitTest.h
//! Included from c++ file to implement unit test. Compatible with M$ unit tests.
//! @note Don't include this from some other header file. Only use in implementation of a test.
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_cUnitTest_H
#define _INC_cUnitTest_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "FileName.h"
#include "StrArg.h"
#include "StrConst.h"
#include "cAppState.h"
#include "cArray.h"
#include "cDebugAssert.h"
#include "cFilePath.h"
#include "cLogLevel.h"
#include "cLogMgr.h"
#include "cOSModImpl.h"
#include "cObjectFactory.h"
#include "cSingleton.h"
#include "cUnitTestDecl.h"  // UNITTEST_FRIEND

namespace Gray {
struct cLogProcessor;

/// <summary>
/// Which level/type of unit tests should we run ?
/// </summary>
enum class UNITTEST_LEVEL_t {
    _None = 0,
    _Crit,    /// 1=critical tests. usually stuff i want to debug now.
    _Core,    /// 2=only the most basic tests.
    _Lib,     /// 3
    _Common,  /// 4 = Common or application level tests.
    _Slow,    /// 5=slow tests
    _All,     /// 6=interactive tests, need special external rigs, db, etc
    _Off,     /// These test don't work yet. or are broken.
};

class cUnitTestRegister;

/// <summary>
/// static struct for context information about the CURRENT running test. singleton
/// Base class for all derived tests cUnitTest
/// Assume we compile in the same environment as we unit test.
/// </summary>
struct GRAYCORE_LINK cUnitTestCur {        // static
    static int sm_nCreatedUnitTests;       /// Count the cUnitTest objects I have created. NOT just m_aUnitTests
    static const FILECHAR_t* k_TestFiles;  // a sub directory under m_sTestOutDir containing all the test files.

    // Sample Test const data.
    static const StrLen_t k_TEXTBLOB_LEN = 566;                 /// StrT::Len(k_sTextBlob) = 0x236
    static const cStrConst k_sTextBlob;                         /// a large test string
    static const cStrConst k_asTextLines[18];  /// nullptr terminated array of lines of text.

    static bool GRAYCALL TestTypes();
};

/// <summary>
/// abstract base unit test for a specific type of thing. attached to a cOSModImpl
/// Similar to something like: JUnit/CppUnit, Boost.Test or CxxTest.
/// Similar to "::Microsoft::VisualStudio::CppUnitTestFramework::TestClass "
///  @note All unit tests should be allowed to run in something like release mode! or be compiled out.
/// </summary>
struct GRAYCORE_LINK cUnitTest : public cObject, public cUnitTestCur {
    cUnitTest();
    ~cUnitTest() noexcept override;  // avoid M$ test use of noexcept(false);

    const FILECHAR_t* get_TestInpDir() const;

    /// Run the test.
    virtual void RunUnitTest() = 0;
};

/// <summary>
/// Hold the registration for a type of cUnitTest. Abstract.
/// ALWAYS constructed in 'C' static init code. The cUnitTest itself is constructed on run demand.
/// Assume static init is NOT multi threaded so no thread locking is required.
/// </summary>
class GRAYCORE_LINK cUnitTestRegister : public cObjectFactory {
 public:
    const UNITTEST_LEVEL_t m_nTestLevel;  /// at what level does this test run?
 protected:
    cUnitTestRegister(const TYPEINFO_t& rTypeInfo, UNITTEST_LEVEL_t nTestLevel = UNITTEST_LEVEL_t::_Core);
    ~cUnitTestRegister() override;

 public:
    void RunUnitTest();  /// Create and run the unit test cUnitTest.
    virtual cUnitTest* CreateUnitTest() const = 0;
};

/// <summary>
/// a singleton to register a unit test for a specific type of thing. Allow creation of its cUnitTest based implementation class.
/// ALWAYS constructed in 'C' static init code. cSingletonStatic< cUnitTestRegisterT<T> >
/// Assume static init is NOT multi threaded so no thread locking is required.
/// </summary>
/// <typeparam name="T"></typeparam>
template <class T = cUnitTest>
struct cUnitTestRegisterT : public cUnitTestRegister, public cSingletonStatic<cUnitTestRegisterT<T> > {
    cUnitTestRegisterT(const TYPEINFO_t& tid, UNITTEST_LEVEL_t nTestLevel = UNITTEST_LEVEL_t::_Core) : cUnitTestRegister(tid, nTestLevel), cSingletonStatic<cUnitTestRegisterT<T> >(this) {}
    /// <summary>
    /// create a derived version of cUnitTest
    /// Never create pure virtual cUnitTest directly (of course).
    /// MUST delete this when done.
    /// </summary>
    /// <returns>cUnitTest base</returns>
    [[nodiscard]] cUnitTest* CreateUnitTest() const override {
        return new T;
    }
    [[nodiscard]] cObject* CreateObject() const override {
        return CreateUnitTest();
    }
};

/// <summary>
/// set to APPSTATE_t::_Run for testing purposes.
/// Unit Tests might use this reentrant with(or without) cAppStateMain ?
/// e.g. cUnitTestAppState inmain;
/// </summary>
class GRAYCORE_LINK cUnitTestAppState {
    cAppState& m_AppState;         /// Fast access to this.
    APPSTATE_t m_eAppStatePrev;    /// Restore the true state of the app if we need to.
    THREADID_t m_nMainThreadPrev;  /// The thread we started with. main().

 public:
    cUnitTestAppState() : m_AppState(cAppState::I()) {
        // called for M$ tests.
        m_eAppStatePrev = m_AppState.get_AppState();
        m_AppState.put_AppState(APPSTATE_t::_Run);
    }

    ~cUnitTestAppState() noexcept {
        m_AppState.put_AppState(m_eAppStatePrev);  // destructors should be called next.
    }
};

/// <summary>
/// Singleton class to hold the list of all unit tests registered.
/// MUST use cSingleton and not cSingletonStatic to prevent C runtime load order problems.
/// </summary>
class GRAYCORE_LINK cUnitTests final : public cSingleton<cUnitTests>, public cUnitTestCur {
    SINGLETON_IMPL(cUnitTests);
    friend class cUnitTestRegister;

 public:
    cArrayPtr<cUnitTestRegister> m_aUnitTests;        /// list of all registered unit tests. Register as they get instantiate by C runtime static loader.
    static AssertCallback_t UnitTest_AssertCallback;  /// redirect assert here for test failure. requires _DEBUG or _DEBUG_FAST.
    AssertCallback_t* m_pAssertOrig = nullptr;        /// restore the original assert.

    UNITTEST_LEVEL_t m_nTestLevel = UNITTEST_LEVEL_t::_Common;  /// The current global test level for UnitTests(). throttle tests at run time.
    cArrayString<LOGCHAR_t> m_aTestNames;  /// just run these tests.

    cFilePath m_sTestInpDir;  /// root for source of test input files. might change based on cOSModImpl?
    cFilePath m_sTestOutDir;  /// global config for input files.

    cLogProcessor* m_pLog = nullptr;  /// cLogMgr::I() for output of tests.	Why not just use DEBUG_MSG ??

    bool m_bRunning = false;  /// We are actively running in the Gray test framework. Not in M$ framework.
    int m_iFailures = 0;      /// Count total unit test failures.

 protected:
    cUnitTests();
    void ReleaseModuleChildren(::HMODULE hMod) override;

 public:
    HRESULT InitTestOutDir();
    bool RegisterUnitTest(cUnitTestRegister* pTest);

    void SetTestLevel(UNITTEST_LEVEL_t nTestLevel);
    bool TestActive(const cUnitTestRegister* pUnitTest, bool remove);

    cUnitTestRegister* FindUnitTest(const char* pszName) const;

    bool IsTestInteractive() const noexcept;
    bool TestInteractivePrompt(const char* pszMsg) noexcept;

    const FILECHAR_t* get_TestInpDir() const;
    const FILECHAR_t* get_TestOutDir() const;

    void RunInitialize();
    void RunCleanup();

    //! Run all tests <= this UNITTEST_LEVEL_t
    HRESULT RunUnitTests(UNITTEST_LEVEL_t nTestLevel = UNITTEST_LEVEL_t::_Common, const LOGCHAR_t* pszTestNameMatch = nullptr);
};

#define UNITTEST_TRUE(x) ASSERTA(x)     // UNITTEST_TRUE is different from a normal ASSERT ? ::Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(x)
#define UNITTEST_TRUE2(x, d) ASSERTA(x)  // UNITTEST_TRUE with a description

// declare a global exposed cUnitTest. Don't use this directly but use UNITTEST2_* to  support M$ test.
#define UNITTEST_REGISTER_NAME(n) g_UnitTest_##n
#define UNITTEST_REGISTER(n, lvl) __DECL_EXPORT cUnitTestRegisterT<UNITTEST_N(n)> UNITTEST_REGISTER_NAME(n)(typeid(UNITTEST_N(n)), lvl)  // instantiate to register cUnitTest  .

// Allow an external hard link to the Base type (because full type is not exposed) and we are pulling from static library. optional.
#define UNITTEST_EXT_NAME(n) g_pUnitTest_##n                                                                    /// a base pointer to cUnitTestRegister for UNITTEST_N(n)
#define UNITTEST_EXT_EXP(n) __DECL_EXPORT cUnitTestRegister* UNITTEST_EXT_NAME(n) = &UNITTEST_REGISTER_NAME(n)  // use this to make the text externally exposed.
#define UNITTEST_EXT_IMP(n) __DECL_IMPORT cUnitTestRegister* UNITTEST_EXT_NAME(n)                               // import = access to externally exposed.

#define UNITTEST_METHOD(x) \
 public:                   \
    void RunUnitTest() override  /// call the public virtual as a test.

}  // namespace Gray

using namespace Gray;  // Since this header is typically only included right before the unit test.
#endif                 // _INC_cUnitTest_H
