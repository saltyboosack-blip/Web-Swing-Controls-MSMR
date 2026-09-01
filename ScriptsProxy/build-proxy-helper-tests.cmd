@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %ERRORLEVEL%
cd /d "%~dp0"
if not exist "x64\ProxyHarness" mkdir "x64\ProxyHarness"
copy /Y "x64\Release\winmm.dll" "x64\ProxyHarness\winmm.dll" >nul
if errorlevel 1 exit /b %ERRORLEVEL%
cl.exe /nologo /std:c++20 /EHsc /W4 /WX /MT proxy_helper_harness.cpp /Fe:"x64\ProxyHarness\proxy-helper.exe" /Fo:"x64\ProxyHarness\\" /link winmm.lib
if errorlevel 1 exit /b %ERRORLEVEL%
copy /Y "x64\ProxyHarness\proxy-helper.exe" "x64\ProxyHarness\crs-video.exe" >nul
copy /Y "x64\ProxyHarness\proxy-helper.exe" "x64\ProxyHarness\crs-handler.exe" >nul
copy /Y "x64\ProxyHarness\proxy-helper.exe" "x64\ProxyHarness\other-helper.exe" >nul
"x64\ProxyHarness\crs-video.exe"
if errorlevel 1 exit /b %ERRORLEVEL%
"x64\ProxyHarness\crs-handler.exe"
if errorlevel 1 exit /b %ERRORLEVEL%
"x64\ProxyHarness\other-helper.exe"
exit /b %ERRORLEVEL%
