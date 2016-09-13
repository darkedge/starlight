@echo off
setlocal

pushd %~dp0
set t0=%time: =0%

make -j%NUMBER_OF_PROCESSORS% "SL_RANDOM=%RANDOM%"

:: Capture the end time before doing anything else
set t=%time: =0%

:: make t0 into a scaler in 100ths of a second, being careful not 
:: to let SET/A misinterpret 08 and 09 as octal
set /a h=1%t0:~0,2%-100
set /a m=1%t0:~3,2%-100
set /a s=1%t0:~6,2%-100
set /a c=1%t0:~9,2%-100
set /a starttime = %h% * 360000 + %m% * 6000 + 100 * %s% + %c%

:: make t into a scaler in 100ths of a second
set /a h=1%t:~0,2%-100
set /a m=1%t:~3,2%-100
set /a s=1%t:~6,2%-100
set /a c=1%t:~9,2%-100
set /a endtime = %h% * 360000 + %m% * 6000 + 100 * %s% + %c%

:: runtime in 100ths is now just end - start
set /a runtime = %endtime% - %starttime%
set runtime = %s%.%c%

:: echo Started at %t0%
echo Build time: %runtime%0 ms

popd
