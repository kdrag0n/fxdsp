#include "sink.h"

namespace fxdsp {

AudioSink::AudioSink(AudioFormat audio_format, int channels) :
        audio_format(audio_format),
        channels(channels) {
    channel_bufs.resize(channels);
}

// Convert S16 LPCM to float and de-interleave channels
bool AudioSink::write_audio_1d(const std::vector<short>& raw_buf) {
    if (audio_format != FORMAT_S16) {
        return false;
    }

    auto samples_per_channel = raw_buf.size() / channels;
    for (auto &channel : channel_bufs) {
        channel.clear();
        channel.resize(samples_per_channel);
    }

    // PCM S16 int -> float32 and de-interleave
    for (auto ch = 0; ch < channels; ch++) {
        for (auto i = 0; i < samples_per_channel; i++) {
            // Combined and duplicated loop for optimal vectorization
            channel_bufs[ch][i] = pcm_s16_to_float32(raw_buf[i * channels + ch]);
        }
    }

    write_audio(channel_bufs);
    return true;
}

bool AudioSink::write_audio_1d(const std::vector<float>& raw_buf) {
    if (audio_format != FORMAT_F32) {
        return false;
    }

    auto samples_per_channel = raw_buf.size() / channels;
    for (auto &channel : channel_bufs) {
        channel.clear();
        channel.resize(samples_per_channel);
    }

    // PCM FORMAT_S16 int -> float32 and de-interleave
    for (auto ch = 0; ch < channels; ch++) {
        for (auto i = 0; i < samples_per_channel; i++) {
            channel_bufs[ch][i] = raw_buf[i * channels + ch];
        }
    }

    write_audio(channel_bufs);
    return true;
}

}
