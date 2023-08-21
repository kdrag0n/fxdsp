#pragma once

#include <vector>

#include "pcm.h"

namespace fxdsp {

class AudioSink {
private:
    std::vector<std::vector<float>> channel_bufs;

public:
    AudioSink(AudioFormat audio_format, int channels);
    virtual ~AudioSink() {}

    AudioFormat audio_format;
    int channels;

    // F32 [channel samples]
    // Implementations can mutate this for efficiency!
    virtual void write_audio(std::vector<std::vector<float>>& buf) = 0;

    // Virtual dispatch doesn't play well with overloading here
    // S16 interleaved
    bool write_audio_1d(const std::vector<short>& raw_buf);
    // F32 interleaved
    bool write_audio_1d(const std::vector<float>& raw_buf);
};

}
