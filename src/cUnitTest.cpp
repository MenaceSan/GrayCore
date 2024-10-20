//! @file cUnitTest.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
// clang-format off
#include "pch.h"
// clang-format on
#include "cAppConsole.h"
#include "cAppImpl.h"
#include "cAppState.h"
#include "cArray.h"
#include "cDebugAssert.h"
#include "cFileCopier.h"
#include "cFileDir.h"
#include "cFilePath.h"
#include "cHeap.h"
#include "cLogEvent.h"
#include "cLogMgr.h"
#include "cPtrTraceMgr.h"
#include "cSingletonPtr.h"
#include "cSystemHelper.h"
#include "cTimeDouble.h"
#include "cTypes.h"
#include "cUniquePtr.h"
#include "cUnitTest.h"

namespace Gray {
cSingleton_IMPL(cUnitTests);

/// <summary>
/// special log file for unit test output.
/// </summary>
class GRAYCORE_LINK cUnitTestLogger : public cLogSink, public cRefBase {
    cFile _File;  // like cLogFileDay

 public:
    IUNKNOWN_DISAMBIG(cRefBase)

    bool CreateLogFile(const FILECHAR_t* pszFileDir) {
        // Create a daily file.
        cTimeUnits tStart;
        tStart.InitTimeNow();
        const cStringF sLogFileName = cStringF::GetFormatf(_FN("UnitTestResults%04d%02d%02d_") _FN(GRAY_COMPILER_NAME) _FN("_") _FN(GRAY_BUILD_NAME) _FN(MIME_EXT_log), tStart._wYear, tStart._wMonth, tStart._wDay);
        const HRESULT hRes = _File.OpenX(cFilePath::CombineFilePathX(pszFileDir, sLogFileName), OF_CREATE | OF_SHARE_DENY_NONE | OF_READWRITE | OF_TEXT);
        return SUCCEEDED(hRes);
    }

    HRESULT addEvent(cLogEvent& rEvent) noexcept override {
        if (_File.isValidHandle()) {
            _File.WriteString(rEvent.get_FormattedDefault());
        }
        return 1;
    }
};

//******************************************************************

int cUnitTestCur::sm_nCreatedUnitTests = 0;

const FILECHAR_t cUnitTestCur::k_TestFiles[] = _FN("TestFiles");

const cStrConst cUnitTestCur::k_asTextLines[18] = {
    // sample test data
    CSTRCONST("four"), CSTRCONST("one"), CSTRCONST("money"), CSTRCONST("two"),   CSTRCONST("red"),   CSTRCONST("blue"), CSTRCONST("get"),  CSTRCONST("go"),     CSTRCONST("to"),
    CSTRCONST("now"),  CSTRCONST("cat"), CSTRCONST("green"), CSTRCONST("ready"), CSTRCONST("three"), CSTRCONST("show"), CSTRCONST("shoe"), CSTRCONST("buckle"), CSTRCONST("my"),
};

const cStrConst cUnitTestCur::k_sTextBlob = CSTRCONST(  // [ cUnitTestCur::k_TEXTBLOB_LEN+1 ]
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

bool GRAYCALL cUnitTestCur::TestTypes() {  // static
    //! Check some basic compiler assumptions.
    //! @note: because c++ has so many compile time options . This should be compiled into any new project context ? make this inline include code ?
    //!
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
    STATIC_ASSERT(sizeof(cUnion32) == 4, cUnion32);  // 32 bits exactly.
    STATIC_ASSERT(sizeof(DWORD) == 4, DWORD);        // _MSC_VER 64=4, _MSC_VER 32=4
    STATIC_ASSERT(sizeof(INT64) == 8, INT64);
    STATIC_ASSERT(sizeof(UINT64) == 8, UINT64);
    STATIC_ASSERT(sizeof(cUnion64) == 8, cUnion64);  // 64 bits exactly.

    const size_t iSizeBool = sizeof(bool);  // _MSC_VER 32=1, _MSC_VER 64=1
    STATIC_ASSERT(iSizeBool == 1, iSizeBool);
    const size_t iSizeEnum = sizeof(enum UNITTEST_LEVEL_t);  // __MSC_VER 32=4, _MSC_VER 64=4
    STATIC_ASSERT(iSizeEnum == 4, iSizeEnum);
    const size_t nSizeWChar = sizeof(wchar_t);  // _MSC_VER 64=2, __MSC_VER 32=2, __GNUC__=4
#ifdef __GNUC__
    STATIC_ASSERT(nSizeWChar == 4, nSizeWChar);  // weird.
#else
    STATIC_ASSERT(nSizeWChar == 2, nSizeWChar);
#endif

    const size_t iSizeFloat = sizeof(float);  // __MSC_VER 32=4, _MSC_VER 64=4
    STATIC_ASSERT(iSizeFloat == 4, iSizeFloat);
    const size_t iSizeDouble = sizeof(double);     // __MSC_VER 32=8, _MSC_VER 64=8
    STATIC_ASSERT(iSizeDouble == 8, iSizeDouble);  // 64 bits

#ifdef USE_LONG_DOUBLE
    const size_t iSizeLongDouble = sizeof(long double);
    // long double = 10 byte, 80 bit float point ? ieee854_float80_t
    STATIC_ASSERT(iSizeLongDouble > 8, iSizeLongDouble);
#endif

#ifdef _WIN32
    const size_t iSizeBool2 = sizeof(BOOL);
    STATIC_ASSERT(iSizeBool2 >= 1, BOOL);  // ?
    STATIC_ASSERT(true == TRUE, TRUE);     // _WIN32 only
    const bool bVal1 = true;
    STATIC_ASSERT(((BYTE)bVal1) == 1, bVal1);
    const bool bVal0 = false;
    STATIC_ASSERT(((BYTE)bVal0) == 0, bVal0);

    const size_t nSizeUINT_PTR = sizeof(UINT_PTR);  // _WIN32=4 _WIN64=8
    STATIC_ASSERT(nSizeUINT_PTR >= 4, nSizeUINT_PTR);
    const size_t nSizeATOM = sizeof(::ATOM);  // _WIN32=2 _WIN64=2
    STATIC_ASSERT(nSizeATOM == 2, nSizeATOM);
    const size_t nSizeHWND = sizeof(::HWND);  // _WIN32=4 _WIN64=8
    STATIC_ASSERT(nSizeHWND >= 4, nSizeHWND);
    const size_t nSizeHCURSOR = sizeof(::HCURSOR);  // _WIN32=4 _WIN64=8
    STATIC_ASSERT(nSizeHCURSOR >= 4, nSizeHCURSOR);
    const size_t nSizePOINT = sizeof(::POINT);  // _WIN32=8 _WIN64=8
    STATIC_ASSERT(nSizePOINT == 8, nSizePOINT);
    const size_t nSizeMSG = sizeof(::MSG);  // _WIN32=28 _WIN64=48
    STATIC_ASSERT(nSizeMSG >= 28, nSizeMSG);
#endif

    const size_t iSizeInt = sizeof(int);  // _MSC_VER 64=4, and _MSC_VER 32=4, __GNUC__ 64=4
    STATIC_ASSERT(iSizeInt == 4, iSizeInt);

    // Test the ambiguous/questionable type.
    const size_t iSizeLong = sizeof(long);  // _MSC_VER 64=4 _MSC_VER 32=4, __GNUC__ 64=8?
#ifdef USE_LONG_AS_INT64                    // 4 or 8 ?
    STATIC_ASSERT(iSizeLong == 8, iSizeLong);
#else
    STATIC_ASSERT(iSizeLong == 4, iSizeLong);
#endif
    STATIC_ASSERT(iSizeLong == _SIZEOF_LONG, _SIZEOF_LONG);
    const size_t iSizeLongInt = sizeof(long int);  // _MSC_VER 64=4, _MSC_VER 32=4
    STATIC_ASSERT(iSizeLongInt == iSizeLong, iSizeLongInt);

    const size_t nSizeLongLong = sizeof(long long);  

    const size_t nSizeSize = sizeof(size_t);  // _MSC_VER 64=8 _MSC_VER 32=4
    STATIC_ASSERT(nSizeSize >= 4, nSizeSize);

#define k_abc "abcdefghijkl"
    STATIC_ASSERT(sizeof(k_abc) == 13, k_abc);    // confirm presumed behavior
    STATIC_ASSERT(_countof(k_abc) == 13, k_abc);  // confirm presumed behavior
    STATIC_ASSERT(STRMAX(k_abc) == 12, k_abc);
#undef k_abc

    const StrLen_t nLenStrA = StrT::Len<char>(k_sTextBlob);
    const StrLen_t nLenStrW = StrT::Len<wchar_t>(k_sTextBlob);
    UNITTEST_TRUE(nLenStrW == nLenStrA);
    UNITTEST_TRUE(nLenStrA == k_sTextBlob._Len);
    UNITTEST_TRUE(nLenStrA == k_TEXTBLOB_LEN);

    // is endian set correctly ?
    cUnion32 u32;
    u32.u_dw = 0x12345678UL;
#ifdef USE_LITTLE_ENDIAN
    UNITTEST_TRUE(u32.u_b[0] == 0x78);
#else
    UNITTEST_TRUE(u32.u_b[0] == 0x12);
#endif

#ifndef __GNUC__
    const size_t iOffset = offsetof(cException, _pszDescription);  // NOTE: Always has a warning using __GNUC__
    STATIC_ASSERT(iOffset > 8, iOffset);                            // check offsetof().
#endif

    // Test some size assumptions. cPtrFacade
    cUniquePtr<int> pNewObj;
    STATIC_ASSERT(sizeof(cUniquePtr<int>) == _SIZEOF_PTR, cUniquePtr);
    STATIC_ASSERT(sizeof(cPtrFacade<int>) == _SIZEOF_PTR, cPtrFacade);

    cRefPtr<> pRefObj;
#ifdef USE_PTRTRACE_REF
    STATIC_ASSERT(sizeof(pRefObj) == _SIZEOF_PTR + sizeof(cPtrTrace), cRefPtr<>);
#else
    STATIC_ASSERT(sizeof(pRefObj) == _SIZEOF_PTR, cRefPtr<>);
#endif

    cIUnkBasePtr pIRefObj;
#ifdef USE_PTRTRACE_IUNK  // may have extra stuff!
    STATIC_ASSERT(sizeof(pIRefObj) == _SIZEOF_PTR + sizeof(cPtrTrace), cIUnkBasePtr);
#else
    STATIC_ASSERT(sizeof(pIRefObj) == _SIZEOF_PTR, cIUnkBasePtr);
#endif

    // Test PtrCastCheck
    cAppState* pTest1 = cAppState::get_Single();
    cAppState* pTest2 = PtrCastCheck<cAppState>(pTest1);
    UNITTEST_TRUE(pTest1 == pTest2);

    return true;
}

//****************************************************************************

cUnitTest::cUnitTest() {
    sm_nCreatedUnitTests++;
}

cUnitTest::~cUnitTest() noexcept {  // avoid M$ test force use of noexcept(false); 	// override
    sm_nCreatedUnitTests--;
}

const FILECHAR_t* cUnitTest::get_TestInpDir() const {
    // my input files my be localized for the test module?
    cUnitTests& uts = cUnitTests::I();
    return uts.get_TestInpDir();
}

//****************************************************************************

cUnitTestRegister::cUnitTestRegister(const TYPEINFO_t& rTypeInfo, UNITTEST_LEVEL_t eTestLevel) : cObjectFactory(rTypeInfo), _eTestLevel(eTestLevel) {
    //! May be constructed in 'C' static init code.
    //! register myself in the unit test list.
    cUnitTests& uts = cUnitTests::I();
    uts.RegisterUnitTest(this);
}

cUnitTestRegister::~cUnitTestRegister() {  // virtual
    //! When a module is released, all its unit tests MUST be destroyed.
    cUnitTests& uts = cUnitTests::I();
    uts._aUnitTests.RemoveArg(this);
}

void cUnitTestRegister::RunUnitTest() {
    //! Create the cUnitTest object and run the cUnitTest.
    ASSERT_NN(this);
    cUniquePtr<cUnitTest> pUnitTest(CreateUnitTest());
    pUnitTest->RunUnitTest();
}

//****************************************************************************

cUnitTests::cUnitTests()
    : cSingleton<cUnitTests>(this),
      _eTestLevel(UNITTEST_LEVEL_t::_Common)  // UNITTEST_LEVEL_t::_Common
{
    cTimePerf::InitFreq();  // make sure this gets called. OK to call again.

    // TODO cRandom g_Rand Seed ?

    // Attach logger.
    _pLog = cLogMgr::get_Single();  // use normal logging nexus by default.

    // Get path to supporting test files
    // e.g. "C:\Dennis\Source\bin\x64v142"
    // _sTestInpDir = cAppState::get_AppFileDir(); // get_CurrentDir();
    _sTestInpDir = cFilePath::GetFileDir(cOSModule(get_HModule(), cOSModule::k_Load_NoRefCount).get_ModulePath());  // track the DLL not the EXE.

#ifdef __linux__
    // _sTestInpDir =_FN("/home/Dennis/grayspace/") _FN(GRAY_NAMES)  _FN(FILESTR_DirSep);	// path for test source files.
#else
    // _sTestInpDir = _FN("C:/Dennis/Source/") _FN(GRAY_NAMES)  _FN(FILESTR_DirSep);	// path for test source files.
#endif
    ASSERT(!_sTestInpDir.IsEmpty());

    _sTestOutDir = cAppState::I().GetTempDir(_FN(GRAY_NAMES));  // create it if needed.
    ASSERT(!_sTestOutDir.IsEmpty());

    // TRy to copy _sTestInpDir files to the _sTestOutDir. Just to see if we have write permissions.
    const HRESULT hRes = CopyTestFiles(get_TestInpDir(), cFilePath::CombineFilePathX(get_TestOutDir(), k_TestFiles));
    if (FAILED(hRes) || hRes < 10 || hRes > 80) {
        DEBUG_ERR(("CopyTestFiles %s", LOGERR(hRes)));
    }
}

void cUnitTests::ReleaseModuleChildren(::HMODULE hMod) {  // override;
    for (ITERATE_t i = _aUnitTests.GetSize(); i;) {
        const cUnitTestRegister* p = _aUnitTests.GetAtCheck(--i);
        if (p == nullptr || p->get_HModule() != hMod) continue;
        _aUnitTests.RemoveAt(i);
    }
}

HRESULT GRAYCALL cUnitTests::CopyTestFiles(const cStringF& srcDir, const cStringF& dstDir) {
    // copy some files from _sTestInpDir to get_TestOutDir to see if we can.

    HRESULT hRes = cFileDir::CreateDirectory1(dstDir);

    int nCountChanges = 0;  // how many files actually copied/changed.
    int nCountFiles = 0;

    cFileFind dir1(srcDir);
    hRes = dir1.FindFile();
    for (; SUCCEEDED(hRes); hRes = dir1.FindFileNext()) {
        // Is this entry something to copy?
        if (dir1.isDots()) continue;                                             // hidden
        if (dir1._FileEntry.get_Name().StartsWithI(_FN(GRAY_NAMES))) continue;  // ignore these.

        // Don't bother copying it if it looks the same.
        cStringF dstPath = cFilePath::CombineFilePathX(dstDir, dir1._FileEntry.get_Name());
        cFileStatus dstStatus;
        hRes = dstStatus.ReadFileStatus(dstPath);
        if (FAILED(hRes) && hRes != HRESULT_WIN32_C(ERROR_FILE_NOT_FOUND)) {
            DEBUG_ERR(("CopyTestFiles Read '%s' = '%s'", LOGSTR(dir1._FileEntry.get_Name()), LOGERR(hRes)));
            break;
        }

        nCountFiles++;
        if (dstStatus.IsFileEqualTo(dir1._FileEntry)) continue;

        hRes = cFileCopier::CopyFileX(dir1.get_FilePath(), dstPath);
        if (FAILED(hRes)) {
            if (hRes == E_ACCESSDENIED) continue;  // just skip it. e.g. 'fine-code-coverage'
            DEBUG_ERR(("CopyTestFiles CopyFileX '%s' = '%s'", LOGSTR(dir1._FileEntry.get_Name()), LOGERR(hRes)));
            break;
        }
        hRes = cFileStatus::WriteFileTimes(dstPath, dir1._FileEntry);
        nCountChanges++;
    }
    if (hRes == HRESULT_WIN32_C(ERROR_NO_MORE_ITEMS)) return nCountFiles;  // nCountChanges;
    return hRes;
}

bool cUnitTests::RegisterUnitTest(cUnitTestRegister* pTest) {
    // add a test that we might run.
    if (_aUnitTests.HasArg3(pTest)) return false;  // no dupes.
    _aUnitTests.Add(pTest);
    return true;
}

cUnitTestRegister* cUnitTests::FindUnitTest(const char* pszName) const {
    //! Find a single test by name.
    for (cUnitTestRegister* pUt : _aUnitTests) {
        if (!StrT::Cmp(pUt->_pszTypeName, pszName)) return pUt;
    }
    return nullptr;
}

bool CALLBACK cUnitTests::UnitTest_AssertCallback(const char* pszExp, const cDebugSourceLine& src) {  // static AssertCallback_t
    //! Assert was called during a unit test. This is a failure!
    //! Assume assert will log this.
    //! AssertCallback_t sm_pAssertCallback = cDebugAssert::Assert_Fail was called in UnitTest. GRAYCALL
    cUnitTests& uts = cUnitTests::I();
    uts._nFailures++;  // Count fail and continue.
    UNREFERENCED_PARAMETER(pszExp);
    UNREFERENCED_REFERENCE(src);
    return true;  // fall through to log and continue.	NOT cDebugAssert::AssertCallbackDefault()
}

const FILECHAR_t* cUnitTests::get_TestOutDir() const {
    return _sTestOutDir.get_CPtr();
}

const FILECHAR_t* cUnitTests::get_TestInpDir() const {
    return _sTestInpDir.get_CPtr();
}

void cUnitTests::SetTestLevel(UNITTEST_LEVEL_t nTestLevel) {
    _eTestLevel = nTestLevel;
    _nFailures = 0;
}

bool cUnitTests::TestActive(const cUnitTestRegister* pUnitTest, bool remove) {
    // Is this test active ?
    if (pUnitTest->_eTestLevel > _eTestLevel) return false;
    if (_aTestNames.isEmpty()) return true;  // allow all.
    // filter _pszTypeName.
    for (int j = 0; j < _aTestNames.GetSize(); j++) {
        if (StrT::MatchRegEx<LOGCHAR_t>(pUnitTest->_pszTypeName, _aTestNames[j], true) > 0) {
            if (remove) _aTestNames.RemoveAt(j);  // found it. NOT a wildcard.
            return true;
        }
    }
    return false;
}

bool cUnitTests::IsTestInteractive() const noexcept {
    if (_eTestLevel <= UNITTEST_LEVEL_t::_Crit) return true;
    if (_eTestLevel >= UNITTEST_LEVEL_t::_All) return true;
#if 0  // def _DEBUG
    // Is debugger attached?
	if (cAppState::isDebuggerPresent()) return true;
#endif
    return false;
}

bool cUnitTests::TestInteractivePrompt(const char* pszMsg) noexcept {
    //! prompt the user to manually check some output from the test.
    //! require user to press key or button.
    //! @return true = continue.

    if (!IsTestInteractive()) return false;
    bool bRet = false;
#ifdef _WIN32
    const int iRet = _GTN(::MessageBox)(WINHANDLE_NULL, StrArg<GChar_t>(pszMsg), _GT("Gray cUnitTest"), MB_OKCANCEL);
    bRet = (iRet == IDOK);
#else  // __linux__
       // console.
    cAppConsole& console = cAppConsole::I();
    console.WriteString(pszMsg);
    int iRet = console.ReadKeyWait();
    bRet = (iRet == 'y');
#endif
    // return input from user.
    return bRet;
}

void cUnitTests::RunInitialize() {
    // Where to send the test output?  Clear the temporary directory ?
    _pAssertOrig = cDebugAssert::sm_pAssertCallback;
    cDebugAssert::sm_pAssertCallback = UnitTest_AssertCallback;  // route asserts back here.

    // configure _pLog with file.
    // Create a results log file with the date stamped on it + Build config. x86 vs x64 etc.
    cRefPtr<cUnitTestLogger> pLogFile(new cUnitTestLogger);
    if (pLogFile->CreateLogFile(_sTestOutDir)) {
        cLogMgr& logger = cLogMgr::I();
        logger.AddSink(pLogFile);
    }

    cLogSinkDebug::AddSinkCheck(nullptr);  // push log to OutputDebugString()
}

void cUnitTests::RunCleanup() {
    // get_TestOutDir()
    cLogMgr& logger = cLogMgr::I();
    logger.RemoveSinkType(typeid(cUnitTestLogger), true);
    cDebugAssert::sm_pAssertCallback = _pAssertOrig;  // restore.
}

HRESULT cUnitTests::RunUnitTests(UNITTEST_LEVEL_t nTestLevel, const LOGCHAR_t* pszTestNameMatch) {
    //! Execute all the registered unit tests at the selected nTestLevel.
    //! @note assume each test is responsible for its own resources.
    //! @arg pszTestNameMatch = a name wildcard. possibly in a comma separated list. (or nullptr)

    SetTestLevel(nTestLevel);

    if (_eTestLevel <= UNITTEST_LEVEL_t::_None) return S_OK;  // tests turned off at run time.

    RunInitialize();

#if defined(_MSC_VER) && defined(_DEBUG) && !defined(UNDER_CE)
    cHeap::Init(_CRTDBG_ALLOC_MEM_DF);
#endif

    _isRunning = true;
    _pLog->addDebugInfoF("cUnitTests input from '%s'", LOGSTR(_sTestInpDir));
    _pLog->addDebugInfoF("cUnitTests output to '%s'", LOGSTR(_sTestOutDir));

    bool isWildCard = true;
    if (pszTestNameMatch == nullptr) {
        _pLog->addDebugInfoF("cUnitTests STARTING %d TESTS at level %d", _aUnitTests.GetSize(), nTestLevel);
        _aTestNames.RemoveAll();  // run All tests
    } else {
        _pLog->addDebugInfoF("cUnitTests STARTING '%s' from %d TESTS", LOGSTR(pszTestNameMatch), _aUnitTests.GetSize());
        _aTestNames.SetStrSep(pszTestNameMatch, ',');  // parse
        isWildCard = StrT::HasRegEx(pszTestNameMatch);
        // _eTestLevel ?
    }

    // Display build/compile info. date, compiler, _MFC_VER/_AFXDLL, 64/32 bit.
    _pLog->addDebugInfoF("Build: '%s' v%d for '%s' on '%s'", GRAY_COMPILER_NAME, GRAY_COMPILER_VER, GRAY_BUILD_NAME, __DATE__);
    // Display current run environment info. OS type, 64/32 bit, OS version, CPU's.
    _pLog->addDebugInfoF("OS: '%s'", LOGSTR(cSystemHelper::I().get_OSInfoStr()));

    const cTimeSys tStart(cTimeSys::GetTimeNow());

    // Test presumed behavior of compiler types.
    TestTypes();

    // Run all s_aUnitTests registered tests at sm_nTestLevel
    char szDashes[64];
    cMem::Fill(szDashes, STRMAX(szDashes), '-');
    szDashes[STRMAX(szDashes)] = '\0';
    UNITTEST_TRUE(!cMem::IsCorruptApp(szDashes, sizeof(szDashes), true));

    // Watch out for memory leaks.
#if defined(USE_PTRTRACE_REF) || defined(USE_PTRTRACE_IUNK)
    cPtrTraceMgr& traceMgr = cPtrTraceMgr::I();
    const ITERATE_t nPtrTrace1 = traceMgr.GetSize();
    cPtrTraceMgr::I().TraceDump(_pLog, 0);
#endif

    ITERATE_t iTestsRun = 0;

    for (cUnitTestRegister* pUnitTest : _aUnitTests) {
        if (!TestActive(pUnitTest, !isWildCard)) continue;

        const StrLen_t iLenName = StrT::Len(pUnitTest->_pszTypeName) + 1;  // room for space
        UNITTEST_TRUE(iLenName > 0 && iLenName < STRMAX(szDashes));
        szDashes[_countof(szDashes) - iLenName] = '\0';

        _pLog->addDebugInfoF("cUnitTest '%s' run  %s", LOGSTR(pUnitTest->_pszTypeName), szDashes);
        szDashes[_countof(szDashes) - iLenName] = '-';

        const cTimePerf tPerfStart(true);  // Time a single test for performance changes.

        pUnitTest->RunUnitTest();

        const cTimePerf tPerfStop(true);  // Time a single test for performance changes.

        if (_nFailures > 0) {
            // Assume cDebugAssert::Assert_Fail was also called.
            _pLog->addDebugInfoF("cUnitTest FAILED Test %d '%s'", iTestsRun, LOGSTR(pUnitTest->get_Name()));
            return E_FAIL;
        }

        iTestsRun++;

        // Do a heap integrity test between unit tests. just in case.
        const bool bHeapCheck = cHeap::Check();
        if (!bHeapCheck && !::Gray::cDebugAssert::Assert_Fail("cHeap::Check", DEBUGSOURCELINE)) {
            // Assume cDebugAssert::Assert_Fail was also called.
            _pLog->addDebugInfoF("cUnitTest FAILED '%s' Heap Corruption.", LOGSTR(pUnitTest->get_Name()));
            return E_FAIL;
        }

#if defined(USE_PTRTRACE_REF) || defined(USE_PTRTRACE_IUNK)
        const ITERATE_t nPtrTrace2 = traceMgr.GetSize();
        if (nPtrTrace2 != nPtrTrace1) {
            _pLog->addDebugInfoF("cUnitTest cPtrTraceMgr Leak %d", nPtrTrace2);
        }
#endif

        // How long did it take?
        ASSERT(tPerfStop.get_Perf() >= tPerfStart.get_Perf());
        const TIMEPERF_t iPerfDiff = tPerfStop.get_Perf() - tPerfStart.get_Perf();
        const double dDaysDiff = cTimePerf::ToDays(iPerfDiff);
        ASSERT(dDaysDiff >= 0 && dDaysDiff < 1);
        const cString sTimeSpan = cTimeDouble::GetTimeSpanStr(dDaysDiff);
        StrLen_t nLenText = sTimeSpan.GetLength() + iLenName + 8;
        if (nLenText > STRMAX(szDashes)) nLenText = STRMAX(szDashes);
        szDashes[_countof(szDashes) - nLenText] = '\0';
        _pLog->addDebugInfoF("cUnitTest '%s' complete in %s %s", LOGSTR(pUnitTest->get_Name()), LOGSTR(sTimeSpan), szDashes);
        szDashes[_countof(szDashes) - nLenText] = '-';
    }

    _isRunning = false;

    _pLog->addDebugInfoF("cUnitTests ENDING %d/%d TESTS in %s", iTestsRun, _aUnitTests.GetSize(), LOGSTR(cTimeInt::GetTimeSpanStr(tStart.get_AgeSec())));

    if (!isWildCard && _aTestNames.GetSize() > 0) {
        // Didn't find some names !
        _pLog->addDebugInfoF("cUnitTest FAILED to find test for '%s'", LOGSTR(_aTestNames[0]));
    }

#if defined(USE_PTRTRACE_IUNK) || defined(USE_PTRTRACE_REF)
    cPtrTraceMgr::I().TraceDump(_pLog, 0);
#endif
    RunCleanup();

    return CastN(HRESULT, iTestsRun);
}
}  // namespace Gray
