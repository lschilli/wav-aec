#!/bin/bash

if [ "$#" -ne 3 ]; then
	echo "Usage $(basename "$0") <near.wav> <far.wav> <out.wav>"
	exit 1
fi

builddir="${BASH_SOURCE[0]%/*}/build"

"$builddir/wav-aec" <(exec sox -R "$1" -r 16000 -t raw -e signed-integer -b 16 - remix 1) <(exec sox -R "$2" -r 16000 -t raw -e signed-integer -b 16 - remix 1) >(exec sox -r 16000 -t raw -e signed-integer -b 16 - "$3")
