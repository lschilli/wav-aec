#!/bin/bash

if [ "$#" -ne 5 ]; then
	echo "Usage $(basename "$0") <near.wav> <channel> <far.wav> <channel> <out.wav> <in_rate> <out_rate>"
	exit 1
fi

in_sr=$4
out_sr=$5
builddir="${BASH_SOURCE[0]%/*}/build"

"$builddir/webrtc-audioproc" -filter_ns -ns_level 3 -in_sr "$in_sr" -out_sr "$out_sr" -near_in <(exec sox -DR "$1" -r $in_sr -t raw -e signed-integer -b 16 - remix "$2") -near_out >(exec sox -DR -r $out_sr -t raw -e signed-integer -b 16 - "$3")
