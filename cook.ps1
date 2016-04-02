# Settings
$win32 = $true

if ($win32) {
    md -Force Win32\bin\assets | Out-Null
    if ((Test-Path external\imgui-1.47\extra_fonts\DroidSans.ttf)) {
        Copy-Item external\imgui-1.47\extra_fonts\DroidSans.ttf -Destination Win32\bin\assets
    } else {
        Write-Host Run install.ps1 first!
        return
    }

    Copy-Item assets\*.* -Destination Win32\bin\assets
}
