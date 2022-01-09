@echo off
REM Setup the build environment, then clean and rebuild the project
REM Expected args are [Debug/Release] and [x86/x64]
REM Working directory is '$(SolutionDir)/$(ProjectDir)/test/'

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\VsMsBuildCmd.bat"
call msbuild.exe /nologo /verbosity:quiet ../../utilext.sln /target:Clean /P:Configuration=%1 /P:Platform=%2
call msbuild.exe /nologo /verbosity:quiet ../../utilext.sln /target:Rebuild /P:Configuration=%1 /P:Platform=%2
