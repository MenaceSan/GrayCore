//! @file NoCRT.inl
//! If we want to NOT use the CRT. We must stub out its functions for each module.
//! @copyright (c) 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#if !USE_CRT
#include "include/cHeap.h"  // cMem

#ifdef _MSC_VER
// M$ will use CRT hidden/embedded in output code.
// https://stackoverflow.com/questions/36627278/compile-c-without-crt/48426180
#define GRAY_CRT_LINK  // GRAYCORE_LINK

// in order to remove use of M$ CRT code. https://www.codeproject.com/articles/15156/tiny-c-runtime-library
// you must:
// 1. NOT use debug build. Release only.
// 3. C/C++: Code Generation: 'Basic Runtime Checks'. Default. /RTC
// 4. C/C++: Code Generation: 'Enable C++ exceptions': NO.  _CPPUNWIND

#pragma comment(linker, "/nodefaultlib:libc.lib")
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/nodefaultlib:libcmtd.lib")

// NOT REQUIRED: ALLOW C/C++: Language: Enable Run-time type information:  No (/GR-)  _CPPRTTI
// NOT REQUIRED: Linker: Input: Ignore all Default Libraries. Disable linking of all standard libraries(/NODEFAULTLIB)

using namespace Gray;

void* operator new(size_t n) {  // throw(std::bad_alloc)
    return cHeap::AllocPtr(n);
}
void operator delete(void* p) throw() {
    cHeap::FreePtr(p);
}
void operator delete(void* p, size_t n) throw() {
    cHeap::FreePtr(p);
}
void operator delete[](void* p) throw() {
    cHeap::FreePtr(p);
}
void operator delete[](void* p, size_t n) throw() {
    cHeap::FreePtr(p);
}

type_info::~type_info() {
    // destruct typeid() info.
}

#ifdef _M_CEE_PURE
System::IntPtr __type_info_root_node;
#else
struct __type_info_node {};
__type_info_node __type_info_root_node;
#endif

typedef void(__cdecl* _PVFV)();

extern "C" {
GRAY_CRT_LINK int _Init_global_epoch = 0;       // epoch_start
__declspec(thread) int _Init_thread_epoch = 0;  // epoch_start

GRAY_CRT_LINK int _fltused = 0x9875;
GRAY_CRT_LINK ULONG _tls_index = 0;  // Thread Local Storage.

// "intrinsic function, cannot be defined"  but are also not provided.

#pragma function(memcmp)
int __cdecl memcmp(const void* b1, const void* b2, size_t n) {
    return cMem::Compare(b1, b2, n);
}

#pragma function(memcpy)
void* __cdecl memcpy(void* dst, const void* src, size_t size) {
    cMem::Copy(dst, src, size);
    return dst;
}

#pragma function(memmove)
void* __cdecl memmove(void* dst, const void* src, size_t count) {
    cMem::CopyOverlap(dst, src, count);
    return dst;
}

#pragma function(memset)
void* __cdecl memset(_Out_writes_bytes_all_(_Size) void* dst, _In_ int val, _In_ size_t size) {
    cMem::Fill(dst, size, val);
    return dst;
}

#ifdef _CPPRTTI
void* __CLRCALL_OR_CDECL __RTtypeid(void* inptr) noexcept(false)  // Pointer to polymorphic object
{
    // https://gist.github.com/cheadaq/1382068
    // https://www.winehq.org/pipermail/wine-cvs/2012-September/089847.html
    // https://www.codeproject.com/script/Content/ViewAssociatedFile.aspx?rzp=%2FKB%2Fsystem%2Fdetect-driver%2F%2FDetectDriverSrc.zip&zep=DetectDriverSrc%2FDetectDriver%2Fsrc%2FdrvCppLib%2Frtti.cpp&obid=58895&obtid=2&ovid=2

    if (inptr == nullptr) {
        // throw ? _CxxThrowException
        return nullptr;
    }

    return nullptr;
}

void* __CLRCALL_OR_CDECL __RTDynamicCast(void* inptr,       // Pointer to polymorphic object
                                         LONG VfDelta,      // Offset of vtable/vfptr in object
                                         void* srcVoid,     // Static type of object pointed to by inptr
                                         void* targetVoid,  // Desired result of cast
                                         BOOL isReference)  // TRUE if input is reference, FALSE if input is ptr
    noexcept(false) {
    return nullptr;
}
#endif

void __cdecl _Init_thread_header(int* const pOnce) noexcept {
    // _Init_thread_epoch
}
void __cdecl _Init_thread_footer(int* const pOnce) noexcept {}

int __cdecl atexit(_PVFV const function) {
    // call a function at exit.
    return 0;
}

void __cdecl __chkstk() {}

size_t __cdecl __std_type_info_hash(__std_type_info_data const* const data) {
    return 0;
}

char const* __cdecl __std_type_info_name(__std_type_info_data* const data, __type_info_node* const root_node) {
    return nullptr;
}

int __cdecl __std_type_info_compare(__std_type_info_data const* const lhs, __std_type_info_data const* const rhs) {
    if (lhs == rhs) {
        return 0;
    }
    // return strcmp(lhs->_DecoratedName + 1, rhs->_DecoratedName + 1);
    return 0;
}

#ifdef _WINDLL
extern "C" BOOL WINAPI _DllMainCRTStartup(HINSTANCE const instance, DWORD const reason, LPVOID const reserved) {
    // Naked entry point for the DLL.
    // TODO: Init and call __DECL_EXPORT BOOL APIENTRY DllMain

    // _Init_global_epoch

    return 0;
}
#endif
}
#endif
#endif
