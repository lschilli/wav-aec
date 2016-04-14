#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/audio_processing/audio_buffer.h"
#include "webrtc/modules/audio_processing/echo_cancellation_impl.h"
#include "webrtc/modules/audio_processing/aec/aec_core_internal.h"

#include "gflags/gflags.h"

#include <iostream>
#include <iomanip>
#include <string.h>
#include <fstream>


using namespace std;
using namespace webrtc;

DEFINE_string(near_in, "", "potentially distorted signal - REQUIRED");
DEFINE_int32(in_sr, 16000, "in sample rate");
DEFINE_string(far_in, "", "reverse signal / aec reference signal");
DEFINE_string(near_out, "", "cleaned near signal - REQUIRED");
DEFINE_int32(out_sr, 16000, "out sample rate");
DEFINE_string(far_out, "", "processed aec reference signal");

DEFINE_int32(sys_delay, 8, "delay between near and far end in ms");
DEFINE_int32(ns_level, -1, "noise supression level 0-3");
DEFINE_int32(aec_level, -1, "aec level 0-2");

//from audioproc_float.cc
DEFINE_bool(filter_aec, false, "Enable echo cancellation.");
DEFINE_bool(filter_agc, false, "Enable automatic gain control.");
DEFINE_bool(filter_hp, false, "Enable high-pass filtering.");
DEFINE_bool(filter_ns, false, "Enable noise suppression.");
DEFINE_bool(filter_ts, false, "Enable transient suppression.");
DEFINE_bool(filter_ie, false, "Enable intelligibility enhancer.");
DEFINE_bool(filter_bf, false, "Enable beamforming.");

DEFINE_bool(aec_delay_agnostic, true, "aec is delay agnostic.");
DEFINE_bool(aec_extended_filter, true, "enable aec extended filter.");

DEFINE_bool(print_stats, false, "print vad/echo/stats.");

template<typename T>
void check_stream_error(const T& stream, const string& filename) {
    if (stream.fail()) {
        cerr << filename << ": " << strerror(errno) << endl;
        exit(1);
    }

}

std::shared_ptr<AudioProcessing> configure_processing() {
    Config config;
    config.Set<ExperimentalNs>(new ExperimentalNs(FLAGS_filter_ts));
    config.Set<Intelligibility>(new Intelligibility(FLAGS_filter_ie));
    if (FLAGS_filter_bf) {
        std::vector<Point> array_geometry;
        array_geometry.push_back(webrtc::Point(-0.1, 0, 0));
        array_geometry.push_back(webrtc::Point(0.1, 0, 0));
        config.Set<Beamforming>(
            new Beamforming(true,
                            array_geometry,
                            SphericalPointf(DegreesToRadians(90), 0.f, 1.f)));
    }
    std::shared_ptr<AudioProcessing> ap(AudioProcessing::Create(config));

    Config extraconfig;
    extraconfig.Set<webrtc::DelayAgnostic>(new webrtc::DelayAgnostic(FLAGS_aec_delay_agnostic));
    extraconfig.Set<webrtc::ExtendedFilter>(new webrtc::ExtendedFilter(FLAGS_aec_extended_filter));
    extraconfig.Set<webrtc::EchoCanceller3>(new webrtc::EchoCanceller3(true));
    ap->SetExtraOptions(extraconfig);

    RTC_CHECK_EQ(AudioProcessing::kNoError, ap->echo_cancellation()->Enable(FLAGS_filter_aec));
    ap->echo_cancellation()->set_suppression_level(EchoCancellation::kHighSuppression);
    if (FLAGS_aec_level != -1) {
        RTC_CHECK_EQ(AudioProcessing::kNoError, ap->echo_cancellation()->set_suppression_level(
                         static_cast<EchoCancellation::SuppressionLevel>(FLAGS_aec_level)));
    }
    ap->echo_cancellation()->enable_metrics(true);
    ap->echo_cancellation()->enable_delay_logging(true);

    RTC_CHECK_EQ(AudioProcessing::kNoError, ap->noise_suppression()->Enable(FLAGS_filter_ns));
    if (FLAGS_ns_level != -1) {
        RTC_CHECK_EQ(AudioProcessing::kNoError, ap->noise_suppression()->set_level(
                         static_cast<NoiseSuppression::Level>(FLAGS_ns_level)));
    }
    RTC_CHECK_EQ(AudioProcessing::kNoError, ap->high_pass_filter()->Enable(FLAGS_filter_hp));
    RTC_CHECK_EQ(AudioProcessing::kNoError, ap->gain_control()->Enable(FLAGS_filter_agc));
    RTC_CHECK_EQ(AudioProcessing::kNoError, ap->gain_control()->set_mode(GainControl::kAdaptiveDigital));

    ap->voice_detection()->Enable(true);
    ap->voice_detection()->set_likelihood(VoiceDetection::kVeryLowLikelihood);
    ap->voice_detection()->set_frame_size_ms(10);
    return ap;
}

void print_configuration(std::shared_ptr<AudioProcessing> ap) {
    cerr << "aec enabled                 " << ap->echo_cancellation()->is_enabled() << "\n";
    if (ap->echo_cancellation()->is_enabled()) {
    cerr << "aec 3 enabled               " << ap->echo_cancellation()->aec_core()->aec3_enabled << "\n"
         << "aec extended filter         " << ap->echo_cancellation()->aec_core()->extended_filter_enabled << "\n"
         << "aec delay agnostic          " << ap->echo_cancellation()->aec_core()->delay_agnostic_enabled << "\n"
         << "aec level                   " << ap->echo_cancellation()->suppression_level() << "\n";
    }
    cerr << "gain control enabled        " << ap->gain_control()->is_enabled() << "\n"
         << "high pass filter_enabled    " << ap->high_pass_filter()->is_enabled() << "\n"
         << "noise suppression enabled   " << ap->noise_suppression()->is_enabled() << "\n"
         << "noise suppression level     " << ap->noise_suppression()->level() << "\n"
         << "voice detection             " << ap->voice_detection()->is_enabled() << endl;
}

int main(int argc, char** argv) {
    google::SetUsageMessage("runs webrtc's audio processing on raw audio files.");
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_near_in.empty() || FLAGS_near_out.empty()) {
        cerr << google::ProgramInvocationShortName() << ": arguments near_in and near_out are required" << endl;
        exit(1);
    }
    if (argc > 1) {
        cerr << google::ProgramInvocationShortName() << ": excess arguments: " << argv[1] << endl;
        exit(1);
    }

    //todo: show aec delays, metrics, speech probability
    const size_t num_chunk_samples_in = FLAGS_in_sr / 100;
    const size_t num_chunk_samples_out = FLAGS_out_sr / 100;
    const int num_channels = 1;

    //near end is mixed signal
    std::ifstream near_in;
    near_in.open(FLAGS_near_in);
    check_stream_error(near_in, FLAGS_near_in);
    //far end is reference signal
    std::ifstream far_in;
    if (!FLAGS_far_in.empty()) {
        far_in.open(FLAGS_far_in);
        check_stream_error(far_in, FLAGS_far_in);
    }
    //filtered signal
    std::ofstream near_out;
    near_out.open(FLAGS_near_out, std::ofstream::out);
    check_stream_error(near_out, FLAGS_near_out);

    //filtered signal
    std::ofstream far_out;
    if (!FLAGS_far_out.empty()) {
        far_out.open(FLAGS_far_out, std::ofstream::out);
        check_stream_error(far_out, FLAGS_far_out);
    }

    std::shared_ptr<AudioProcessing> ap = configure_processing();
    print_configuration(ap);

    std::vector<int16_t> far_raw_data(num_chunk_samples_in * num_channels);
    std::vector<int16_t> near_raw_data(num_chunk_samples_in * num_channels);
    std::vector<int16_t> out_raw_data(num_chunk_samples_out * num_channels);
    std::vector<int16_t> far_out_raw_data(num_chunk_samples_out * num_channels);
    std::vector<float> far_float_data(num_chunk_samples_in * num_channels);
    std::vector<float> near_float_data(num_chunk_samples_in * num_channels);
    std::vector<float> out_float_data(num_chunk_samples_out * num_channels);
    std::vector<float> far_out_float_data(num_chunk_samples_out * num_channels);
    webrtc::ChannelBuffer<float> far_chan_buf(num_chunk_samples_in, num_channels);
    webrtc::ChannelBuffer<float> near_chan_buf(num_chunk_samples_in, num_channels);
    webrtc::ChannelBuffer<float> out_chan_buf(num_chunk_samples_out, num_channels);
    webrtc::ChannelBuffer<float> far_out_chan_buf(num_chunk_samples_out, num_channels);

    webrtc::StreamConfig stream_config_in(FLAGS_in_sr, num_channels);
    webrtc::StreamConfig stream_config_out(FLAGS_out_sr, num_channels);
    int buf_cnt = 0;
    while (true) {
        if (!FLAGS_far_in.empty()) {
            far_in.read((char*)far_raw_data.data(), far_raw_data.size()*sizeof(int16_t));
            if (!far_in) break;
            webrtc::S16ToFloat(far_raw_data.data(), far_raw_data.size(), far_float_data.data());
            webrtc::Deinterleave(far_float_data.data(), num_chunk_samples_in, num_channels, far_chan_buf.channels());
        }
        near_in.read((char*)near_raw_data.data(), near_raw_data.size()*sizeof(int16_t));
        if (!near_in) break;
        webrtc::S16ToFloat(near_raw_data.data(), near_raw_data.size(), near_float_data.data());
        webrtc::Deinterleave(near_float_data.data(), num_chunk_samples_in, num_channels, near_chan_buf.channels());
        if (!FLAGS_far_in.empty()) {
            RTC_CHECK_EQ(AudioProcessing::kNoError,
                         ap->set_stream_delay_ms(FLAGS_sys_delay));
            RTC_CHECK_EQ(AudioProcessing::kNoError,
                         ap->ProcessReverseStream(far_chan_buf.channels(), stream_config_in,
                                                  stream_config_out, far_out_chan_buf.channels()));
        }

        RTC_CHECK_EQ(AudioProcessing::kNoError,
                     ap->ProcessStream(near_chan_buf.channels(), stream_config_in,
                                       stream_config_out, out_chan_buf.channels()));

        webrtc::Interleave(out_chan_buf.channels(), out_chan_buf.num_frames(),
                           out_chan_buf.num_channels(), out_float_data.data());

        webrtc::FloatToS16(out_float_data.data(), out_raw_data.size(), out_raw_data.data());
        near_out.write((char*)out_raw_data.data(), out_raw_data.size()*sizeof(int16_t));

        if (!FLAGS_far_out.empty()) {
            webrtc::Interleave(far_out_chan_buf.channels(), far_out_chan_buf.num_frames(),
                           far_out_chan_buf.num_channels(), far_out_float_data.data());
            webrtc::FloatToS16(far_out_float_data.data(), far_out_raw_data.size(), far_out_raw_data.data());
            far_out.write((char*)far_out_raw_data.data(), far_out_raw_data.size()*sizeof(int16_t));
        }
        buf_cnt++;

        if (FLAGS_print_stats) {
            int delay_std, delay_med;
            float delay_poor;
            ap->echo_cancellation()->GetDelayMetrics(&delay_med, &delay_std, &delay_poor);
            AecCore* aec = ap->echo_cancellation()->aec_core();
            float nlpout = sqrt(aec->nlpoutlevel.framelevel.GetLatestMean() - aec->nlpoutlevel.minlevel);
            //float nlpout = static_cast<float>(10 * log10(aec->nlpoutlevel.framelevel.GetLatestMean() - aec->nlpoutlevel.minlevel + 1e-10f));

            cout << "t: " << showpoint << (buf_cnt*0.01)
                 << " voice: " << ap->voice_detection()->stream_has_voice()
                 << " echo: " << ap->echo_cancellation()->stream_has_echo()
                 << " voiceprob: " << showpoint<< ap->noise_suppression()->speech_probability()
                 << " nlplevel: " << showpoint<< nlpout
                 << " delaymed: " << delay_med
                 << " delaystd: " << delay_std
                 << " delaypoor: " << delay_poor
                 << endl;
        }

    }

    near_in.close();
    far_in.close();
    near_out.close();

}



