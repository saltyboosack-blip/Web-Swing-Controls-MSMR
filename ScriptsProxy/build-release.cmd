@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0generate-winmm-forwarders.ps1"
if errorlevel 1 exit /b %ERRORLEVEL%
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" "%~dp0ScriptsProxy.vcxproj" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m:1 /verbosity:minimal
exit /b %ERRORLEVEL%
