$shell = New-Object -com Shell.Application
$webClient = New-Object Net.WebClient
$externalDir = "external"

$workingDir = $((Resolve-Path .).ToString() + "\")
Write-Host $("Working directory: " + $workingDir)

# Convenience functions
function DownloadFile($address) {
	$file = Split-Path $address -Leaf
	$dest = $($workingDir + $file)
	if ((Test-Path $dest)) {
		Write-Host $("Found " + $file)
		return
	} else {
		Write-Host $("Downloading " + $file + "...")
		$webClient.DownloadFile($address, $dest)
	}
}

function DownloadAndExtract($address) {
	$file = Split-Path $address -Leaf
	$dest = $($workingDir + $file)
	if ((Test-Path $dest)) {
		Write-Host $("Found " + $file)
	} else {
		Write-Host $("Downloading " + $file + "...")
		$webClient.DownloadFile($address, $dest)
	}

	$extension = [System.IO.Path]::GetExtension($file)
	Write-Host $("Extracting " + $file + "...")
	if ($extension -eq ".zip") {
		$cmd = ".\7za.exe x $file -o$externalDir -aos"
        Write-Host $cmd
		Invoke-Expression $cmd
	} elseif ($extension -eq ".gz") { # .tar.gz
        $cmd = ".\7za.exe e $file -aos"
        Write-Host $cmd
        Invoke-Expression $cmd

        $tar = [System.IO.Path]::GetFileNameWithoutExtension($file)
        $cmd = ".\7za.exe x $tar -o$externalDir -aos"
        Write-Host $cmd
        Invoke-Expression $cmd
	} else {
		Write-Host $($file + ": Unsupported archive format!")
	}
}

DownloadFile("http://www.7-zip.org/a/7za920.zip")

Write-Host Extracting 7-Zip executable...
foreach ($item in $shell.NameSpace($($workingDir + "7za920.zip")).Items()) {
    if ($item.Name -eq "7za.exe") {
        # 16 = "Respond with "Yes to All" for any dialog box that is displayed."
        $shell.NameSpace($workingDir).CopyHere($item, 16)
    }
}

DownloadAndExtract("http://enet.bespin.org/download/enet-1.3.13.tar.gz")
DownloadAndExtract("https://github.com/g-truc/glm/releases/download/0.9.7.2/glm-0.9.7.2.zip")
DownloadAndExtract("https://github.com/ocornut/imgui/archive/v1.47.zip")
DownloadAndExtract("http://www.lua.org/ftp/lua-5.1.5.tar.gz")
DownloadAndExtract("https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.zip")
DownloadAndExtract("https://github.com/google/flatbuffers/archive/v1.3.0.zip")

# Delete downloads
Remove-Item "7za.exe" -Recurse -ErrorAction Ignore
Remove-Item "7za920.zip" -Recurse -ErrorAction Ignore
Remove-Item "enet-1.3.13.tar*" -Recurse -ErrorAction Ignore
Remove-Item "glm-0.9.7.2.zip" -Recurse -ErrorAction Ignore
Remove-Item "lua-5.1.5.tar*" -Recurse -ErrorAction Ignore
Remove-Item "protobuf-2.6.1.zip" -Recurse -ErrorAction Ignore
Remove-Item "v1.3.0.zip" -Recurse -ErrorAction Ignore
Remove-Item "v1.47.zip" -Recurse -ErrorAction Ignore
