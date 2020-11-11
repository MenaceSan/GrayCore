//
//! @file cWinHeap.cpp
//! @copyright 1992 - 2020 Dennis Robinson (http://www.menasoft.com)
//

#include "pch.h"
#include "cWinHeap.h"

#if defined(_WIN32)

#if USE_UNITTESTS
#include "cUnitTest.h"
UNITTEST_CLASS(cWinHeap)
{
	UNITTEST_METHOD(cWinHeap)
	{
		// Enum all the private heaps for this process.

#ifndef UNDER_CE
		HANDLE aProcessHeaps[256];
		DWORD dwRet = ::GetProcessHeaps(_countof(aProcessHeaps), aProcessHeaps);
		UNITTEST_TRUE(dwRet > 0);

		for (DWORD i = 0; i < dwRet; i++)
		{
			cWinHeap heap(aProcessHeaps[i], false);
			if (!heap.isValidHeap())
			{
				sm_pLog->addInfoF("cWinHeap 0%x Not valid", heap.get_Handle());
				continue;
			}

			ULONG HeapInformation = 0;
			SIZE_T nSizeData = sizeof(HeapInformation);
			// HEAP_INFORMATION_CLASS = 1 = HeapEnableTerminationOnCorruption
			heap.QueryInformation((HEAP_INFORMATION_CLASS)1, &HeapInformation, &nSizeData);

			sm_pLog->addInfoF("cWinHeap 0%x compact=%d, term=%d", heap.get_Handle(), heap.Compact(), HeapInformation);
		}
#endif
	}
};
UNITTEST_REGISTER(cWinHeap, UNITTEST_LEVEL_Lib);
#endif
#endif