
$shell = new-object -com shell.application
$zip = '7za920.zip'
if (!(Test-Path .\7za920.zip)) {
	Write-Host "Downloading 7-Zip..."
	(New-Object Net.WebClient).DownloadFile('http://www.7-zip.org/a/7za920.zip','.\7za920.zip')
}
$dir = $((Resolve-Path .).ToString() + "\")
$join = Join-Path $dir $zip
Write-Host "Extracting 7-Zip..."
foreach ($item in $shell.namespace($join).Items()){

    if ($item.Name -eq "7za.exe") {
        $shell.Namespace($dir).copyhere($item)
    }
}
