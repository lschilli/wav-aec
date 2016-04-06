#!/bin/bash

if [ "$#" -ne 7 ]; then
	echo "Usage $(basename "$0") <near.wav> <channel> <far.wav> <channel> <out.wav> <in_rate> <out_rate>"
	exit 1
fi

in_sr=$6
out_sr=$7
builddir="${BASH_SOURCE[0]%/*}/build"

"$builddir/webrtc-audioproc" -filter_aec -aec_level 2 -filter_ns -ns_level 1 -in_sr "$in_sr" -out_sr "$out_sr" -near_in <(exec sox -DR "$1" -r $in_sr -t raw -e signed-integer -b 16 - remix "$2") -far_in <(exec sox -DR "$3" -r $in_sr -t raw -e signed-integer -b 16 - remix "$4") -near_out >(exec sox -DR -r $out_sr -t raw -e signed-integer -b 16 - "$5")
