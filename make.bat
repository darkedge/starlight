@echo off
pushd %~dp0
where cl.exe || call "%VS140COMNTOOLS:~0,-14%VC\vcvarsall.bat" amd64
jom /j %NUMBER_OF_PROCESSORS% /nologo "SL_RANDOM=%RANDOM%"
popd
