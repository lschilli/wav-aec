#!/bin/bash

if [ "$#" -ne 5 ]; then
	echo "Usage $(basename "$0") <near.wav> <channel> <far.wav> <channel> <out.wav>"
	exit 1
fi

samplerate=48000
builddir="${BASH_SOURCE[0]%/*}/build"

"$builddir/webrtc-audioproc" -filter_aec -aec_level 2 -filter_ns -ns_level 1 -near_in_sr 48000 -near_in <(exec sox -DR "$1" -r $samplerate -t raw -e signed-integer -b 16 - remix "$2") -far_in <(exec sox -DR "$3" -r $samplerate -t raw -e signed-integer -b 16 - remix "$4") -near_out >(exec sox -DR -r $samplerate -t raw -e signed-integer -b 16 - "$5")
