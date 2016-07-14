@echo off
pushd %~dp0
jom /j %NUMBER_OF_PROCESSORS% /nologo "SL_RANDOM=%RANDOM%"
popd
