# Get working directory
# http://stackoverflow.com/questions/59895/can-a-bash-script-tell-which-directory-it-is-stored-in
workingDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Output folder
externalDir="$workingDir/external"

# Create external folder
if [ ! -d "$externalDir" ]; then
	echo "Creating folder \"$externalDir\"..."
	mkdir $externalDir
fi

# Convenience functions
function DownloadAndExtract {
	local file=$(basename $1)
	local dest="$workingDir/$file"
	if [ -e "$dest" ]; then
		echo "Found $file"
	else
		echo "Downloading $1..."
		curl -sSL $1 > $dest
	fi
	
	local extension="${file##*.}"
	echo "Extracting $file..."
	if [ "$extension" == "zip" ]; then
		unzip -o $dest -d $externalDir
	else
		if [ "$extension" == "gz" ]; then
			tar -zxf $dest -C $externalDir
		else
			echo "$file: Unsupported archive format!"
		fi
	fi

	# TODO: extraction filters
}

DownloadAndExtract http://enet.bespin.org/download/enet-1.3.13.tar.gz
DownloadAndExtract https://github.com/ocornut/imgui/archive/v1.49.zip
DownloadAndExtract https://github.com/google/protobuf/releases/download/v2.6.1/protobuf-2.6.1.zip
DownloadAndExtract https://github.com/erwincoumans/sce_vectormath/archive/master.zip

# Delete downloads
function DeleteFile {
	rm -f $workingDir/$1
}

DeleteFile enet-1.3.13.tar*
DeleteFile protobuf-2.6.1.zip
DeleteFile v1.49.zip
DeleteFile master.zip
