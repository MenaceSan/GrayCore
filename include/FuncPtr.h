//! @file FuncPtr.h
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)

#ifndef _INC_FuncPtr_H
#define _INC_FuncPtr_H
#ifndef NO_PRAGMA_ONCE
#pragma once
#endif

#include "cMem.h"

#if defined(__GNUC__)
typedef int(GRAYCALL* FARPROC)();  // like _WIN32 FARPROC
typedef void* FUNCPTR_t;           // a Generic function pointer. like dlsym(). Never call this directly!
#else
typedef void* FUNCPTR_t;  // a Generic function pointer. FARPROC. Never call this directly!
#endif

namespace Gray {
/// <summary>
/// Convert function pointer to another type. gcc doesnt allow cast of func pointer to other types!?
/// </summary>
template <typename T, typename TFP>  // FUNCPTR_t
static T CastFPtrTo(TFP p) {
    return (T)p;
}

static inline bool IsValidFunction(const void* p) {
    return p != nullptr;
}

}  // namespace Gray
#endif
