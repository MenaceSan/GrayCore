rem Assume we have built all the binaries. 
rem for DebugDLL, ReleaseDLL, ReleaseStat, DebugStat x Win32, x64
rem TODO msbuild buildapp.csproj -t:HelloWorld -p:Configuration=Release
rem This will copy them all to the appropriate folder structure to prepare for NuGet packing.
rem Assume NuGet tools are installed /Programs/NuGet.exe .
rem https://digitalhouseblog.wordpress.com/2019/08/22/how-to-make-a-nuget-package-for-c/
rem https://docs.microsoft.com/en-us/nuget/reference/nuget-exe-cli-reference -> https://www.nuget.org/downloads
rem use for v141 or v142
@ECHO OFF
IF "%ToolVer%" == "" set ToolVer=v142
set CodeVer=1.6.6
set BinDir=..\..\..\bin
set DstDir=%BinDir%\graycore-%ToolVer%.%CodeVer%
@ECHO on

rem clean dir.
del "%DstDir%\" /Q
rmdir /s/q "%DstDir%\"
md "%DstDir%\"

xcopy /Q /R /Y "..\ReadMe.md" "%DstDir%\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "..\License.txt" "%DstDir%\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "GrayWorldLogoSm.jpg" "%DstDir%\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "graycore-%ToolVer%.nuspec" "%DstDir%\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED

md "%DstDir%\build\"
copy ".\GrayCore.nuget.props" "%DstDir%\build\graycore-%ToolVer%.props"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
rem copy ".\GrayCore.nuget.targets" "%DstDir%\build\graycore-%ToolVer%.targets"
rem IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem copy build helper file for static lib linking.
copy ".\Directory.Build.props" "%DstDir%\*"
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem md %DstDir%\bin\
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\DebugDLL\GrayCore.dll" "%DstDir%\bin\Win32\DebugDLL\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\ReleaseDLL\GrayCore.dll" "%DstDir%\bin\Win32\ReleaseDLL\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\DebugDLL\GrayCore.dll" "%DstDir%\bin\x64\DebugDLL\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\ReleaseDLL\GrayCore.dll" "%DstDir%\bin\x64\ReleaseDLL\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem md %DstDir%\include\
xcopy /i /Q /R /Y "..\include\*.*" "%DstDir%\include\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem md %DstDir%\lib\
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\DebugDLL\GrayCore.lib" "%DstDir%\lib\Win32\DebugDLL\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\DebugDLL\GrayCore.pdb" "%DstDir%\lib\Win32\DebugDLL\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\DebugStat\GrayCore.lib" "%DstDir%\lib\Win32\DebugStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\DebugStat\GrayCore.pdb" "%DstDir%\lib\Win32\DebugStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\ReleaseDLL\GrayCore.lib" "%DstDir%\lib\Win32\ReleaseDLL\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\ReleaseDLL\GrayCore.pdb" "%DstDir%\lib\Win32\ReleaseDLL\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\ReleaseStat\GrayCore.lib" "%DstDir%\lib\Win32\ReleaseStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\Win32%ToolVer%\ReleaseStat\GrayCore.pdb" "%DstDir%\lib\Win32\ReleaseStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\DebugDLL\GrayCore.lib" "%DstDir%\lib\x64\DebugDLL\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\DebugDLL\GrayCore.pdb" "%DstDir%\lib\x64\DebugDLL\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\DebugStat\GrayCore.lib" "%DstDir%\lib\x64\DebugStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\DebugStat\GrayCore.pdb" "%DstDir%\lib\x64\DebugStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\ReleaseDLL\GrayCore.lib" "%DstDir%\lib\x64\ReleaseDLL\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\ReleaseDLL\GrayCore.pdb" "%DstDir%\lib\x64\ReleaseDLL\"
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\ReleaseStat\GrayCore.lib" "%DstDir%\lib\x64\ReleaseStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED
xcopy /Q /R /Y "%BinDir%\x64%ToolVer%\ReleaseStat\GrayCore.pdb" "%DstDir%\lib\x64\ReleaseStat\" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem now zip it up.
rem e.g. /Programs/NuGet.exe pack GrayCore-v142.nuspec -BasePath "C:\Dennis\Source\bin\graycore-v142.1.6.6" -OutputDirectory "C:\Dennis\Source\bin"
/Programs/NuGet.exe pack GrayCore-%ToolVer%.nuspec -BasePath "%DstDir%" -OutputDirectory "%BinDir%" 
IF ERRORLEVEL 1 GOTO ERRORDETECTED

rem add to local test server.
rem /Programs/NuGet.exe add graycore-v142.1.6.6.nupkg -source C:\Dennis\Source\bin\packages
/Programs/NuGet.exe add "%BinDir%\graycore-%ToolVer%.%CodeVer%.nupkg" -source "%BinDir%\packages"
IF ERRORLEVEL 1 GOTO ERRORDETECTED

ECHO SUCCESS !

GOTO DONE

:ERRORDETECTED
ECHO ERROR DETECTED
PAUSE
rem EXIT

:DONE
