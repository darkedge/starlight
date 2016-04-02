# Delete Visual Studio output folders
Remove-Item "Win32-Debug" -Recurse -ErrorAction Ignore
Remove-Item "Win32-Release" -Recurse -ErrorAction Ignore
Remove-Item "x64-Debug" -Recurse -ErrorAction Ignore
Remove-Item "x64-Release" -Recurse -ErrorAction Ignore

# Delete runtime generated files (logs/configs)
Remove-Item "imgui.ini" -ErrorAction Ignore
