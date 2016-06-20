@echo off
pushd %~dp0
set Destination=%1
if [%1]==[] set Destination="vs\Win32\bin\assets"

mkdir %Destination%

:: Copy imgui font file
if not exist external\imgui-1.49\extra_fonts\DroidSans.ttf (
	echo Run install script first!
	goto end
)
xcopy external\imgui-1.49\extra_fonts\*.ttf %Destination% /Y /Q > NUL

:: Copy assets to destination folder
xcopy assets\*.* %Destination% /Y /Q /S > NUL

:end
popd
