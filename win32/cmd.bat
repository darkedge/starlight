@echo off
PATH=%PATH%;%JAVA_HOME%\bin
cmd.exe /k call "%VS140COMNTOOLS:~0,-14%VC\vcvarsall.bat" amd64
