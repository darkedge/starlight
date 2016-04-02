.\clean.ps1
$cmd = "git clean -d -f -f -X"
Invoke-Expression $cmd
