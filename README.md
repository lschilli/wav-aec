# wav-aec

* This tool applies [webrtc's](https://webrtc.org/) acoustic echo cancellation (AEC) functionality to wav files.
* sox is used for wav -> raw conversion
* Building a full webrtc release is avoided. Only the signal processing functions are needed.

Cloning including the webrtc code:
```bash
git clone --recursive https://github.com/lschilli/wav-aec.git
```

After cloning:
```bash
git submodule update --init --recursive
```
