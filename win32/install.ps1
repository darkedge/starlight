# This saves typing
$shell = New-Object -com Shell.Application
$webClient = New-Object Net.WebClient

# -aoa: Overwrite all without prompt
# -r: Recursive
$7zargs = " -aoa -r *.cpp *.hpp *.c *.h *.inl *.ttf jom.exe"

# Get working directory
$workingDir = $((Split-Path -Path $MyInvocation.MyCommand.Definition -Parent) + "\")

# Output folder
$externalDir = $($workingDir + "..\external")

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
		Write-Host $("Downloading " + $address + "...")
		$webClient.DownloadFile($address, $dest)
	}

	$extension = [System.IO.Path]::GetExtension($file)
	Write-Host $("Extracting " + $file + "...")
	if ($extension -eq ".zip") {
		$cmd = $($workingDir + "7za.exe x " + $dest + " -o" + $externalDir + $7zargs)
        Write-Host $cmd
		Invoke-Expression $cmd
	} elseif ($extension -eq ".gz") { # .tar.gz
        $cmd = $($workingDir + "7za.exe e " + $dest + " -o$workingDir -aoa")
        Write-Host $cmd
        Invoke-Expression $cmd

        $tar = $($workingDir + [System.IO.Path]::GetFileNameWithoutExtension($file))
        $cmd = $($workingDir + "7za.exe x $tar -o$externalDir $7zargs")
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

# jom
DownloadFile("http://download.qt.io/official_releases/jom/jom.zip")
foreach ($item in $shell.NameSpace($($workingDir + "jom.zip")).Items()) {
    if ($item.Name -eq "jom.exe") {
        # 16 = "Respond with "Yes to All" for any dialog box that is displayed."
        $shell.NameSpace($workingDir).CopyHere($item, 16)
    }
}

DownloadAndExtract("http://enet.bespin.org/download/enet-1.3.13.tar.gz")
DownloadAndExtract("https://github.com/ocornut/imgui/archive/v1.49.zip")
DownloadAndExtract("https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.zip")
DownloadAndExtract("https://github.com/erwincoumans/sce_vectormath/archive/master.zip")

# Delete downloads
function DeleteFile($file) {
    Remove-Item $($workingDir + $file) -Recurse -ErrorAction Ignore
}

DeleteFile("7za.exe")
DeleteFile("7za920.zip")
DeleteFile("enet-1.3.13.tar*")
DeleteFile("protobuf-2.6.1.zip")
DeleteFile("v1.49.zip")
DeleteFile("master.zip")
DeleteFile("jom.zip")
