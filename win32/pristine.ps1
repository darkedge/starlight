function Execute($cmd) {
    Write-Host $cmd
    Invoke-Expression $cmd
}

# Makefile clean target
if ((Test-Path "make")) {
    Execute("make clean")
}

# Add git to path
if ((Test-Path $($env:ProgramFiles + "\Git\bin"))) {
    $env:Path += $(";" + $env:ProgramFiles + "\Git\bin")
} elseif ((Test-Path $(${env:ProgramFiles(x86)} + "\Git\bin"))) {
    $env:Path += $(";" + ${env:ProgramFiles(x86)} + "\Git\bin")
}

pushd ..
Execute("git clean -d -f -f -X")
popd
