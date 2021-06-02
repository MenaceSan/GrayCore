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
# ADD CPP /nologo /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D GRAYCORE_LINK= /Yu"pch.h" /FD /c
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
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D GRAYCORE_LINK= /FR /Yu"pch.h" /FD /GZ /c
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

SOURCE=.\cArray.h
# End Source File
# Begin Source File

SOURCE=.\CArrayRef.h
# End Source File
# Begin Source File

SOURCE=.\cArraySort.h
# End Source File
# Begin Source File

SOURCE=.\cArrayString.cpp
# End Source File
# Begin Source File

SOURCE=.\cArrayString.h
# End Source File
# Begin Source File

SOURCE=.\cAtom.cpp
# End Source File
# Begin Source File

SOURCE=.\cAtom.h
# End Source File
# Begin Source File

SOURCE=.\CException.cpp
# End Source File
# Begin Source File

SOURCE=.\cException.h
# End Source File
# Begin Source File

SOURCE=.\CFile.cpp
# End Source File
# Begin Source File

SOURCE=.\CFile.h
# End Source File
# Begin Source File

SOURCE=.\cFileDir.cpp
# End Source File
# Begin Source File

SOURCE=.\cFileDir.h
# End Source File
# Begin Source File

SOURCE=.\cFilePath.cpp
# End Source File
# Begin Source File

SOURCE=.\cFilePath.h
# End Source File
# Begin Source File

SOURCE=.\cFileText.cpp
# End Source File
# Begin Source File

SOURCE=.\cFileText.h
# End Source File
# Begin Source File

SOURCE=.\cHeapBlock.cpp
# End Source File
# Begin Source File

SOURCE=.\cHeapBlock.h
# End Source File
# Begin Source File

SOURCE=.\cHeapValidate.h
# End Source File
# Begin Source File

SOURCE=.\cIniFile.cpp
# End Source File
# Begin Source File

SOURCE=.\cIniFile.h
# End Source File
# Begin Source File

SOURCE=.\cIniObject.cpp
# End Source File
# Begin Source File

SOURCE=.\cIniObject.h
# End Source File
# Begin Source File

SOURCE=.\CLogBase.cpp
# End Source File
# Begin Source File

SOURCE=.\CLogBase.h
# End Source File
# Begin Source File

SOURCE=.\cUniquePtr.h
# End Source File
# Begin Source File

SOURCE=.\cNonCopyable.h
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

SOURCE=.\cSingleton.h
# End Source File
# Begin Source File

SOURCE=.\cSingletonMT.h
# End Source File
# Begin Source File

SOURCE=.\cStream.h
# End Source File
# Begin Source File

SOURCE=.\cStreamQueue.h
# End Source File
# Begin Source File

SOURCE=.\CString.cpp
# End Source File
# Begin Source File

SOURCE=.\cString.h
# End Source File
# Begin Source File

SOURCE=.\CStringSys.h
# End Source File
# Begin Source File

SOURCE=.\cSystemInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\cSystemInfo.h
# End Source File
# Begin Source File

SOURCE=.\cThreadLock.cpp
# End Source File
# Begin Source File

SOURCE=.\cThreadLock.h
# End Source File
# Begin Source File

SOURCE=.\cThreadLockX.h
# End Source File
# Begin Source File

SOURCE=.\cThreadSafeLong.h
# End Source File
# Begin Source File

SOURCE=.\CTime.cpp
# End Source File
# Begin Source File

SOURCE=.\CTime.h
# End Source File
# Begin Source File

SOURCE=.\cTimeDouble.cpp
# End Source File
# Begin Source File

SOURCE=.\cTimeDouble.h
# End Source File
# Begin Source File

SOURCE=.\cTimeSys.cpp
# End Source File
# Begin Source File

SOURCE=.\cTimeSys.h
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

SOURCE=.\pch.cpp
# ADD CPP /Yc"pch.h"
# End Source File
# Begin Source File

SOURCE=.\pch.h
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
