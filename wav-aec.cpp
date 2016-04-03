#include "webrtc/modules/audio_processing/aec/echo_cancellation.h"
#include "webrtc/modules/audio_processing/aec/aec_core.h"

#include <iostream>
#include <string.h>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;

namespace po = boost::program_options;


po::variables_map parse_opts(int argc, char** argv) {
    try {
        po::positional_options_description positionalOptions;
        positionalOptions.add("near-sig", 1);
        positionalOptions.add("far-sig", 1);
        positionalOptions.add("out-sig", 1);
        po::options_description allopts("");
        po::options_description desc("Usage: wav-aec [options] <near-sig> <far-sig> <out-sig>\noptions are");
        desc.add_options()
                ("help", "show this help message")
                ("sys-delay", po::value<int>()->default_value(12), "set system delay (device buffer). strong effect if not configured delay agnostic")
                ("nlp-mode", po::value<int>()->default_value(2), "set the nlp level (0-2)")
                ("disable-delay-agnostic", "disable automatic delay estimation")
                ("disable-extended-filter", "less postprocessing")
                ("show-delay", "show delay estimation info")
                ;
        allopts.add(desc);
        allopts.add_options()
                ("near-sig", po::value<string>()->required(), "signal")
                ("far-sig", po::value<string>()->required(), "signal")
                ("out-sig", po::value<string>()->required(), "signal");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(allopts).positional(positionalOptions).run(), vm);

        if (vm.count("help")) {
            cout << desc << endl;
            exit(1);
        }
        po::notify(vm);
        return vm;
    }catch(po::error& e) {
        cerr << "Error parsing command line:" << endl << e.what() << endl;
        exit(1);
    }
}

template<typename T>
void check_stream_error(const T& stream, const string& filename) {
    if (stream.fail()) {
        cerr << filename << ": " << strerror(errno) << endl;
        exit(1);
    }

}


int main(int argc, char** argv) {

    po::variables_map vm(parse_opts(argc, argv));
    bool show_delay = vm.count("show-delay");
    string f_near_sig(vm["near-sig"].as<string>());
    string f_far_sig(vm["far-sig"].as<string>());
    string f_out_sig(vm["out-sig"].as<string>());

    const int kDeviceBufMs = vm["sys-delay"].as<int>(); //param delay, strong effect if not delay agnostic
    const int kSamplesPerChunk = 160;
    float far_[kSamplesPerChunk];
    float near_[kSamplesPerChunk];
    float out_[kSamplesPerChunk];
    const float* near_ptr_ = near_;
    float* out_ptr_ = out_;
    int16_t rawaudiobuf[kSamplesPerChunk];

    webrtc::AecConfig aeconfig;
    if (show_delay) {
        aeconfig.delay_logging = webrtc::kAecTrue;
    } else {
        aeconfig.delay_logging = webrtc::kAecFalse;
    }
    //webrtc::kAecNlpConservative ... webrtc::kAecNlpAggressive
    aeconfig.nlpMode = vm["nlp-mode"].as<int>();
    if (aeconfig.nlpMode > 2 || aeconfig.nlpMode < 0) {
        cerr << "invalid nlp mode" << endl;
        exit(1);
    }
    aeconfig.metricsMode= webrtc::kAecFalse;
    aeconfig.skewMode = webrtc::kAecFalse;

    void* aecInst = webrtc::WebRtcAec_Create();
    int ret = webrtc::WebRtcAec_Init(aecInst, 16000, 16000);
    if (ret) {
        cerr << "WebRtcAec_Init: Error " << ret << endl;
        exit(1);
    }
    webrtc::WebRtcAec_set_config(aecInst, aeconfig);
    webrtc::AecCore* aeccore = webrtc::WebRtcAec_aec_core(aecInst);
    webrtc::WebRtcAec_enable_aec3(aeccore, 1);
    if (vm.count("disable-extended-filter")) {
        webrtc::WebRtcAec_enable_extended_filter(aeccore, 0);
    } else {
        webrtc::WebRtcAec_enable_extended_filter(aeccore, 1);
    }
    if (vm.count("disable-delay-agnostic")) {
        webrtc::WebRtcAec_enable_delay_agnostic(aeccore, 0);
    } else {
        webrtc::WebRtcAec_enable_delay_agnostic(aeccore, 1);
    }
    //near end is mixed signal
    std::ifstream near_in;
    near_in.open (f_near_sig);
    check_stream_error(near_in, f_near_sig);
    //far end is aec reference signal
    std::ifstream far_in;
    far_in.open (f_far_sig);
    check_stream_error(far_in, f_far_sig);
    //filtered signal
    std::ofstream aec_out;
    aec_out.open(f_out_sig, std::ofstream::out);
    check_stream_error(aec_out, f_out_sig);

    int buf_cnt = 0;
    while (true) {
        far_in.read((char*)rawaudiobuf, kSamplesPerChunk*sizeof(int16_t));
        std::copy(rawaudiobuf, rawaudiobuf+kSamplesPerChunk, far_);
        near_in.read((char*)rawaudiobuf, kSamplesPerChunk*sizeof(int16_t));
        std::copy(rawaudiobuf, rawaudiobuf+kSamplesPerChunk, near_);

        if (!far_in || !near_in) break;
        check_stream_error(near_in, f_near_sig);
        check_stream_error(far_in, f_far_sig);

        ret = webrtc::WebRtcAec_BufferFarend(aecInst, far_, kSamplesPerChunk);
        if (ret) {
            cerr << "WebRtcAec_BufferFarend: Error " << ret << endl;
            exit(1);
        }

        ret = webrtc::WebRtcAec_Process(aecInst, &near_ptr_, 1, &out_ptr_,
                                        kSamplesPerChunk, kDeviceBufMs, 0);
        if (ret) {
            cerr << "WebRtcAec_Process: Error " << ret << endl;
            exit(1);
        }

        for (int i = 0; i < kSamplesPerChunk; i++) {
            rawaudiobuf[i] = out_[i];
        }
        aec_out.write((char*)rawaudiobuf, kSamplesPerChunk*sizeof(int16_t));
        check_stream_error(aec_out, f_out_sig);

        if (show_delay) {
            int delay_med;
            int delay_std;
            float delay_poor;
            webrtc::WebRtcAec_GetDelayMetrics(aecInst, &delay_med, &delay_std, &delay_poor);
            cout << "t:\t" << (buf_cnt*0.01) << "\tmedian:\t" <<  delay_med << "\tpercent_poor:\t" << delay_poor << endl;
        }
        buf_cnt++;
    }
    near_in.close();
    far_in.close();
    aec_out.close();
}

