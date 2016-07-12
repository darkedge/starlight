@echo off
pushd %~dp0
jom /j %NUMBER_OF_PROCESSORS% "SL_RANDOM=%RANDOM%"
popd
