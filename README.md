/*! @mainpage Gray C++ Libraries Main Page
# GrayCore
C++ core library providing very basic utility functionality without using any STL. Minimal dependencies.

GRAY_VERSION_S = Version 1.6.7

https://www.menasoft.com/graydox/d8/df5/namespaceGray.html  
https://github.com/MenaceSan/GrayCore  
https://www.nuget.org/packages?q=graycore  

The Gray libraries are a set of source code modules written mostly in C++ over 10 years or so and used in a variety of projects.
They are compilable in MSVC and GNU compilers and will run in several flavors of Windows, WinCE, Linux.

Main libraries are:
@verbatim
GrayCore,  
GrayLib,  
Gray3D,  
GrayCodec,  
GrayDotNet,  
GrayGraph,  
GrayGUI,  
GrayJSX,  
GrayKernel for Windows kernel drivers/services,  
GrayLUA,  
GrayMapData,  
GrayPerfMon  
@endverbatim

Main applications are:
@verbatim
GrayAgent,
GrayBrowser,
GrayFTP,
GrayServer,
GrayX
@endverbatim

@section graycore GrayCore Library

Core classes used by all other modules. Make code more portable and more object oriented.
GrayCore has very minimal external library requirements. Only make use of standard C lib and core OS modules.
Abstract and minimize the direct use of OS services such that a program can compile for Windows, Linux and Windows CE. 
Code can build with various tools and environments such as x64, x86, GNU tools, Microsoft compilers, MFC or just std C, Unicode or UTF8 default.
Unit Tests are in a separate project called GrayCore.Tests 

@verbatim
Includes conditional compilation for _MSC_VER, __GNUC__, _WIN32, __linux__, _AFXDLL/_MFC_VER, UNDER_CE, _M_X64, _UNICODE.
Linux, Windows or WinCE/PocketPC (Embedded windows).
Visual Studio and Eclipse projects.
GNU C++ (GCC 5.1), MSVC 7.0(2003),8.0(2005),10.0(2010),14.0(2015),15.0(2017),16.0(2019). (_MSC_VER=MSVC)
64 or 32 bit, UNICODE or UTF8, MFC, stdlib/crt (optional STL), or POSIX.
Compile as DLL(SO) or static library.
@endverbatim

Provides common OS wrapper services:
@verbatim
Event/Debug Logging and Filtering. Inspired by LogForJ. Topics, Priorities, Filters and routing.
String, (Atoms), UTF8/Unicode.
Time, Date and High Precision Time (for Profiling).
Array, Sorted Arrays, Hash Tables.
Thread Locking and Interlocked data types.
File, Text and Binary.
File folder lists. File name/path validation.
Smart (reference counted) Pointers to heap.
Heap management.
Hooking of existing functions on x86,64 machines.
Singleton, NonCopyable classes. Lazy construction.
Streams, Queues.
Unicode/UTF8 convert.
Error Codes, Error messages as HRESULT codes.
System/OS Info.
Ini file read/write for application configuration.
Exceptions abstraction for STL or MFC exceptions.
Mime type name manager.
@endverbatim

STYLE GUIDE:
@verbatim
Declare things before you use them. Just like auto stack variables are declared before you use them. properties/fields should be declared before use in inline methods for consistency.
Allman style (BSD in Emacs), BSD style code format (in eclipse). { at start of new line etc. (aka Microsoft default style).
Use M$ naming style: https://msdn.microsoft.com/en-us/library/windows/desktop/aa378932%28v=vs.85%29.aspx
Use c* prefix for Class to differentiate from MFC style where there might be a conflict. So we can mix namespaces.
avoid "#define" constants by using static const values and enums.
avoid magic numbers and magic strings embedded in code.
"#include" only on demand. don't "#include" anything not immediately used.
"#include" all needed refs such that order of "#include" is never a problem.
Avoid project include paths if possible. Put more informative path into "#include". "#include "GrayCore/x.h"" except for "#include <system.h>" files.
Declare modular dependencies loudly. Use "#pragma lib" in MSVC.
Avoid use of STL. It has poor implementation in M$ world and is cryptic.
Minimal use of standard C libraries. May have platform dependent issues. CRT versions. etc.
Don't care about single function exit point. Conserve indentation. return when done.
Tab indentation.
Use .tbl files to equate static data sets with enums.
a 'struct' is used for bare structures (POCO w/o methods), interfaces and static classes (use namespace?). a 'class' is used for all others. e.g. things that can be instantiated and have methods.
define typedef SUPER_t to (mostly) replace M$ specific __super keyword.
Any function named IsValid* should never throw. use noexcept.
@endverbatim

C++11 keywords used: override, final, auto, decltype, static_assert, nullptr

Doxygen Help:
https://www.menasoft.com/graydox/d8/df5/namespaceGray.html
http://www.stack.nl/~dimitri/doxygen/commands.html#cmdfile
use http://www.die.net for Linux man docs.

Build for Linux:

Needs dl, pthread
g++ -std=c++0x -D_DEBUG -O2 -g -Wall -c -fmessage-length=0 -fPIC -pthread -o *

NuGet packages:

https://www.nuget.org/packages?q=graycore
Build native NuGet package for *Release  and *debug configs and multiple tool chains/CRT versions and platforms

graycore-v141.1.6.7 = separate package for the v141 tool chain. VS2017
graycore-v142.1.6.7 = separate package for the v142 tool chain. VS2019

Contains options for:
Configuration = *release* or *debug*  
Platform = Win32 or x64
GRAY_STATICLIB = Static lib else DLL/SO  (default)   

No options for:
consume CRT as a dll only. 
stdcall calling convention only. 
NO_CRT
MFC

@todo
USE_CRT = 0 = minimize use of CRT MSVCRTD . remove or wrap -> sprintf, strtod, rand, malloc ?
Timeunits : span merge duplicated code.
ThreadLock RW,
Log message cache for details messages. Released if not needed.
CreateObject and service locator/creator by type name
vsprintfN overflow return for Linux and Win32 is not quite right/consistent. -1 or max or new size?
Log messages that require response/ack. not duplicated, revokable, UID.
_CPPRTTI = for dynamic_cast ? not avail in UNDER_CE
_CPPUNWIND = for exceptions. not avail in UNDER_CE
__GNUC__ 64 bit sync exchange in 32 bit code. _InterlockedCompareExchange64
Heap:Alloc() fail should not always assert. this may not always be fatal/unexpected.
__linux__ getopt() like argument definitions for apps.
Test UNICODE vs binary vs ASCII cFileText files. WriteString() and SeekX().
enumerate devices/drives mounted on system.
LINUX write file attributes? chmod() for particular user.
Test - thread handling for cString. can multiple threads use instances of the same string ? instance interlock count delete works OK?

*/
