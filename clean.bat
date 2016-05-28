@echo off
if exist Win32 rmdir Win32 /s /q
if exist build rmdir build /s /q
call %~dp0\cook.bat
