@echo off
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /format:list') do set datetime=%%I
set datetime=%datetime:~0,8%-%datetime:~8,6%

if not exist export mkdir export
pushd export

if not exist %datetime% mkdir %datetime%
pushd %datetime%

mkdir bin
popd
popd

xcopy ..\assets export\%datetime%\assets\ /Y /Q

xcopy starlight.dll export\%datetime%\bin /Y /Q
xcopy lua51.dll export\%datetime%\bin /Y /Q
xcopy starlight.exe export\%datetime%\bin /Y /Q
