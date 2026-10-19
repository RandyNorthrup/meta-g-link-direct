@echo off
setlocal
rem Removes the Meta G Link Direct plugin and setup tool. Close OBS first.
rem The stored glasses identity is kept; delete %LOCALAPPDATA%\MetaGLinkDirect to forget it.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-MetaGLinkDirect.ps1" -Uninstall
set "status=%errorlevel%"
echo.
pause
exit /b %status%
