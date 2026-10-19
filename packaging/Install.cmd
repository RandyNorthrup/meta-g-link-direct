@echo off
setlocal
rem Installs Meta G Link Direct into OBS Studio. Close OBS first.
rem Optional: -Setup skips the y/N question and goes to the glasses setup (you still type YES);
rem -Uninstall removes the plugin and the setup tool.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-MetaGLinkDirect.ps1" %*
set "status=%errorlevel%"
echo.
pause
exit /b %status%
