rem cd C:\Dennis\Source\Gray\GrayCore\build 
rem VS160COMCOMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\
rem x86 x64

call "%VS160COMCOMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x64
rem  call "%VS160COMCOMNTOOLS%..\..\VC\Auxiliary\Build\vcvarsall.bat" x86

rem call "%VS160COMCOMNTOOLS%..\IDE\devenv.exe" %~dp0\..\GrayCore16.sln /Build ReleaseDLL

msbuild.exe %~dp0\..\GrayCore16.sln /t:Build /p:Configuration=Release;Platform=Win32
 
rem TODO projects/Config/Targets from Batch Build.
