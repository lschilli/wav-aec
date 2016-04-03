#!/bin/bash

if [ "$#" -ne 5 ]; then
	echo "Usage $(basename "$0") <near.wav> <channel> <far.wav> <channel> <out.wav>"
	exit 1
fi

builddir="${BASH_SOURCE[0]%/*}/build"

"$builddir/wav-aec" <(exec sox -R "$1" -r 16000 -t raw -e signed-integer -b 16 - remix "$2") <(exec sox -R "$3" -r 16000 -t raw -e signed-integer -b 16 - remix "$4") >(exec sox -r 16000 -t raw -e signed-integer -b 16 - "$5")
