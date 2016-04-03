cmake_minimum_required(VERSION 2.8)

PROJECT(wav-aec)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
add_definitions("-DWEBRTC_LINUX -DWEBRTC_POSIX")
INCLUDE_DIRECTORIES(webrtc)

FIND_PACKAGE(Boost COMPONENTS system chrono program_options REQUIRED)

FUNCTION(PREPEND_PATH var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND_PATH)

SET(WEBRTC_SRC_
    common_audio/real_fourier.cc
    common_audio/fir_filter.cc
    common_audio/wav_file.cc
    common_audio/window_generator.cc
    common_audio/signal_processing/cross_correlation.c
    common_audio/signal_processing/levinson_durbin.c
    common_audio/signal_processing/ilbc_specific_functions.c
    common_audio/signal_processing/min_max_operations.c
    common_audio/signal_processing/refl_coef_to_lpc.c
    common_audio/signal_processing/real_fft.c
    common_audio/signal_processing/splitting_filter.c
    common_audio/signal_processing/spl_sqrt_floor.c
    common_audio/signal_processing/filter_ma_fast_q12.c
    common_audio/signal_processing/resample_fractional.c
    common_audio/signal_processing/resample_by_2_internal.c
    common_audio/signal_processing/dot_product_with_scale.c
    common_audio/signal_processing/auto_correlation.c
    common_audio/signal_processing/randomization_functions.c
    common_audio/signal_processing/copy_set_operations.c
    common_audio/signal_processing/filter_ar.c
    common_audio/signal_processing/resample.c
    common_audio/signal_processing/complex_fft.c
    common_audio/signal_processing/division_operations.c
    common_audio/signal_processing/spl_init.c
    common_audio/signal_processing/auto_corr_to_refl_coef.c
    common_audio/signal_processing/resample_48khz.c
    common_audio/signal_processing/get_hanning_window.c
    common_audio/signal_processing/filter_ar_fast_q12.c
    common_audio/signal_processing/spl_sqrt.c
    common_audio/signal_processing/downsample_fast.c
    common_audio/signal_processing/resample_by_2.c
    common_audio/signal_processing/get_scaling_square.c
    common_audio/signal_processing/vector_scaling_operations.c
    common_audio/signal_processing/energy.c
    common_audio/signal_processing/lpc_to_refl_coef.c
    common_audio/signal_processing/sqrt_of_one_minus_x_squared.c
    common_audio/signal_processing/complex_bit_reverse.c
    common_audio/resampler/sinc_resampler_sse.cc
    common_audio/resampler/push_sinc_resampler.cc
    common_audio/resampler/sinusoidal_linear_chirp_source.cc
    common_audio/resampler/resampler.cc
    common_audio/resampler/push_resampler.cc
    common_audio/resampler/sinc_resampler.cc
    common_audio/ring_buffer.c
    common_audio/audio_ring_buffer.cc
    common_audio/sparse_fir_filter.cc
    common_audio/real_fourier_ooura.cc
    common_audio/lapped_transform.cc
    common_audio/audio_util.cc
    common_audio/fir_filter_sse.cc
    common_audio/fft4g.c
    common_audio/channel_buffer.cc
    common_audio/wav_header.cc
    common_audio/audio_converter.cc
    common_audio/blocker.cc
    base/checks.cc
    system_wrappers/source/aligned_malloc.cc
    base/criticalsection.cc
    base/platform_thread.cc
    base/thread_checker_impl.cc
    base/event.cc
    modules/audio_processing/aec/aec_rdft.c
    modules/audio_processing/aec/aec_core.cc
    modules/audio_processing/aec/aec_rdft_sse2.c
    modules/audio_processing/aec/aec_core_sse2.cc
    modules/audio_processing/aec/echo_cancellation.cc
    modules/audio_processing/echo_cancellation_impl.cc
    modules/audio_processing/audio_buffer.cc
    modules/audio_processing/logging/aec_logging_file_handling.cc
    modules/audio_processing/splitting_filter.cc
    modules/audio_processing/three_band_filter_bank.cc
    system_wrappers/source/cpu_features.cc
    modules/audio_processing/utility/block_mean_calculator.cc
    modules/audio_processing/utility/delay_estimator_wrapper.c
    modules/audio_processing/aec/aec_resampler.cc
    modules/audio_processing/utility/delay_estimator.c
)

PREPEND_PATH(WEBRTC_SRC webrtc/webrtc ${WEBRTC_SRC_})

ADD_EXECUTABLE(wav-aec wav-aec.cpp ${WEBRTC_SRC})
target_link_libraries(wav-aec ${Boost_LIBRARIES} pthread)
