# Microsoft Developer Studio Project File - Name="GrayCore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=GrayCore - Win32 Debug6Stat
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GrayCore6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GrayCore6.mak" CFG="GrayCore - Win32 Debug6Stat"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GrayCore - Win32 Release6Stat" (based on "Win32 (x86) Static Library")
!MESSAGE "GrayCore - Win32 Debug6Stat" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GrayCore - Win32 Release6Stat"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release6Stat"
# PROP BASE Intermediate_Dir "Release6Stat"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release6"
# PROP Intermediate_Dir "Release6"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D GRAYCORE_LINK= /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "GrayCore - Win32 Debug6Stat"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug6Stat"
# PROP BASE Intermediate_Dir "Debug6Stat"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug6"
# PROP Intermediate_Dir "Debug6"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D GRAYCORE_LINK= /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "GrayCore - Win32 Release6Stat"
# Name "GrayCore - Win32 Debug6Stat"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CArray.h
# End Source File
# Begin Source File

SOURCE=.\CArrayRef.h
# End Source File
# Begin Source File

SOURCE=.\CArraySort.h
# End Source File
# Begin Source File

SOURCE=.\CArrayString.cpp
# End Source File
# Begin Source File

SOURCE=.\CArrayString.h
# End Source File
# Begin Source File

SOURCE=.\CAtom.cpp
# End Source File
# Begin Source File

SOURCE=.\CAtom.h
# End Source File
# Begin Source File

SOURCE=.\CException.cpp
# End Source File
# Begin Source File

SOURCE=.\CException.h
# End Source File
# Begin Source File

SOURCE=.\CFile.cpp
# End Source File
# Begin Source File

SOURCE=.\CFile.h
# End Source File
# Begin Source File

SOURCE=.\CFileDir.cpp
# End Source File
# Begin Source File

SOURCE=.\CFileDir.h
# End Source File
# Begin Source File

SOURCE=.\CFilePath.cpp
# End Source File
# Begin Source File

SOURCE=.\CFilePath.h
# End Source File
# Begin Source File

SOURCE=.\CFileText.cpp
# End Source File
# Begin Source File

SOURCE=.\CFileText.h
# End Source File
# Begin Source File

SOURCE=.\CHeapBlock.cpp
# End Source File
# Begin Source File

SOURCE=.\CHeapBlock.h
# End Source File
# Begin Source File

SOURCE=.\CHeapValidate.h
# End Source File
# Begin Source File

SOURCE=.\CIniFile.cpp
# End Source File
# Begin Source File

SOURCE=.\CIniFile.h
# End Source File
# Begin Source File

SOURCE=.\CIniObject.cpp
# End Source File
# Begin Source File

SOURCE=.\CIniObject.h
# End Source File
# Begin Source File

SOURCE=.\CLogBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CLogBase.h
# End Source File
# Begin Source File

SOURCE=.\CNewPtr.h
# End Source File
# Begin Source File

SOURCE=.\CNonCopyable.h
# End Source File
# Begin Source File

SOURCE=.\CProfileFunc.cpp
# End Source File
# Begin Source File

SOURCE=.\CProfileFunc.h
# End Source File
# Begin Source File

SOURCE=.\CQueueBytes.cpp
# End Source File
# Begin Source File

SOURCE=.\CQueueBytes.h
# End Source File
# Begin Source File

SOURCE=.\CRefPtr.h
# End Source File
# Begin Source File

SOURCE=.\CSingleton.h
# End Source File
# Begin Source File

SOURCE=.\CSingletonMT.h
# End Source File
# Begin Source File

SOURCE=.\CStream.h
# End Source File
# Begin Source File

SOURCE=.\CStreamQueue.h
# End Source File
# Begin Source File

SOURCE=.\CString.cpp
# End Source File
# Begin Source File

SOURCE=.\CString.h
# End Source File
# Begin Source File

SOURCE=.\CStringSys.h
# End Source File
# Begin Source File

SOURCE=.\CSystemInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\CSystemInfo.h
# End Source File
# Begin Source File

SOURCE=.\CThreadLock.cpp
# End Source File
# Begin Source File

SOURCE=.\CThreadLock.h
# End Source File
# Begin Source File

SOURCE=.\CThreadLockX.h
# End Source File
# Begin Source File

SOURCE=.\CThreadSafeLong.h
# End Source File
# Begin Source File

SOURCE=.\CTime.cpp
# End Source File
# Begin Source File

SOURCE=.\CTime.h
# End Source File
# Begin Source File

SOURCE=.\CTimeDouble.cpp
# End Source File
# Begin Source File

SOURCE=.\CTimeDouble.h
# End Source File
# Begin Source File

SOURCE=.\CTimeSys.cpp
# End Source File
# Begin Source File

SOURCE=.\CTimeSys.h
# End Source File
# Begin Source File

SOURCE=.\DateBase.cpp
# End Source File
# Begin Source File

SOURCE=.\DateBase.h
# End Source File
# Begin Source File

SOURCE=.\GrayCore.cpp
# End Source File
# Begin Source File

SOURCE=.\GrayCore.h
# End Source File
# Begin Source File

SOURCE=.\hResultCodes.tbl
# End Source File
# Begin Source File

SOURCE=.\hResults.cpp
# End Source File
# Begin Source File

SOURCE=.\hResults.h
# End Source File
# Begin Source File

SOURCE=.\hResults.tbl
# End Source File
# Begin Source File

SOURCE=.\IRefPtr.cpp
# End Source File
# Begin Source File

SOURCE=.\IRefPtr.h
# End Source File
# Begin Source File

SOURCE=.\IUnknown.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"StdAfx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StringUnicode.cpp
# End Source File
# Begin Source File

SOURCE=.\StringUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\StringUtil.h
# End Source File
# Begin Source File

SOURCE=.\StringUtilMod.cpp
# End Source File
# Begin Source File

SOURCE=.\StringUtilT.cpp
# End Source File
# End Group
# End Target
# End Project
