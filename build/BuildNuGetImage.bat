rem Assume we have built all the binaries. This will copy them all to the appropriate fodler structure to prepare for NuGet packing.

@ECHO OFF
set CodeVer=1.6.4
set ToolVer=v141
set BinDir=..\..\..\bin
set DstDir=%BinDir%\graycore-%ToolVer%.%CodeVer%
@ECHO on

rem md %DstDir%
xcopy  /i /C /Q /R /Y "..\ReadMe.md" %DstDir%\
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y "..\License.txt" %DstDir%\
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y "graycore-%ToolVer%.nuspec" %DstDir%\
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem md %DstDir%\bin\
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\DebugDLL\GrayCore.dll %DstDir%\bin\Win32\DebugDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\ReleaseDLL\GrayCore.dll %DstDir%\bin\Win32\ReleaseDLL\
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\DebugDLL\GrayCore.dll %DstDir%\bin\x64\DebugDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\ReleaseDLL\GrayCore.dll %DstDir%\bin\x64\ReleaseDLL\
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem md %DstDir%\build\
xcopy  /i /C /Q /R /Y "GrayCore.nuget.props" %DstDir%\build\
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem md %DstDir%\include\
xcopy  /i /C /Q /R /Y "..\include\*.*" %DstDir%\include\
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem md %DstDir%\lib\
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\DebugDLL\GrayCore.lib %DstDir%\lib\Win32\DebugDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\DebugDLL\GrayCore.pdb %DstDir%\lib\Win32\DebugDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\DebugStat\GrayCore.lib %DstDir%\lib\Win32\DebugStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\DebugStat\GrayCore.pdb %DstDir%\lib\Win32\DebugStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\ReleaseDLL\GrayCore.lib %DstDir%\lib\Win32\ReleaseDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\ReleaseDLL\GrayCore.pdb %DstDir%\lib\Win32\ReleaseDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\ReleaseStat\GrayCore.lib %DstDir%\lib\Win32\ReleaseStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\Win32%ToolVer%\ReleaseStat\GrayCore.pdb %DstDir%\lib\Win32\ReleaseStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\DebugDLL\GrayCore.lib %DstDir%\lib\x64\DebugDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\DebugDLL\GrayCore.pdb %DstDir%\lib\x64\DebugDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\DebugStat\GrayCore.lib %DstDir%\lib\x64\DebugStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\DebugStat\GrayCore.pdb %DstDir%\lib\x64\DebugStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\ReleaseDLL\GrayCore.lib %DstDir%\lib\x64\ReleaseDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\ReleaseDLL\GrayCore.pdb %DstDir%\lib\x64\ReleaseDLL\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\ReleaseStat\GrayCore.lib %DstDir%\lib\x64\ReleaseStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy  /i /C /Q /R /Y %BinDir%\x64%ToolVer%\ReleaseStat\GrayCore.pdb %DstDir%\lib\x64\ReleaseStat\ 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem now zip it up.
EXIT

:ERRORDETECTED
ECHO ERROR DETECTED
PAUSE
EXIT
