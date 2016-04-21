.\clean.ps1

# Add git to path
if ((Test-Path $($env:ProgramFiles + "\Git\bin"))) {
    $env:Path += $(";" + $env:ProgramFiles + "\Git\bin")
} elseif ((Test-Path $(${env:ProgramFiles(x86)} + "\Git\bin"))) {
    $env:Path += $(";" + ${env:ProgramFiles(x86)} + "\Git\bin")
}

$cmd = "git clean -d -f -f -X"
Invoke-Expression $cmd
