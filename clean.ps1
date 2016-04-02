# Delete Visual Studio output files
Remove-Item "Win32-Debug" -Recurse -ErrorAction Ignore
Remove-Item "Win32-Debug-Lib" -Recurse -ErrorAction Ignore
Remove-Item "Win32-Release" -Recurse -ErrorAction Ignore
Remove-Item "Win32-Release-Lib" -Recurse -ErrorAction Ignore
Remove-Item "x64-Debug" -Recurse -ErrorAction Ignore
Remove-Item "x64-Debug-Lib" -Recurse -ErrorAction Ignore
Remove-Item "x64-Release" -Recurse -ErrorAction Ignore
Remove-Item "x64-Release-Lib" -Recurse -ErrorAction Ignore
