@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %ERRORLEVEL%
cd /d "%~dp0"
if not exist "x64\HostGateTests" mkdir "x64\HostGateTests"
cl.exe /nologo /std:c++20 /EHsc /W4 /WX host_gate_tests.cpp host_gate.cpp /Fe:"x64\HostGateTests\HostGateTests.exe" /Fo:"x64\HostGateTests\\"
if errorlevel 1 exit /b %ERRORLEVEL%
"x64\HostGateTests\HostGateTests.exe"
exit /b %ERRORLEVEL%
