#include <algorithm>
#include <cmath>

#include "pcm.h"

namespace fxdsp {

float pcm_s16_to_float32(short sample) {
    // TODO: consider reciprocal multiplication for performance
    return static_cast<float>(sample) / 32768.0f;
}

short pcm_float32_to_s16(float sample) {
    // This is trickier than expected (32768 vs 32767 in the forward and inverse conversions):
    //   - http://www.mega-nerd.com/libsndfile/FAQ.html#Q010
    //   - http://blog.bjornroche.com/2009/12/int-float-int-its-jungle-out-there.html
    //   - https://www.kvraudio.com/forum/viewtopic.php?t=414666
    //   - https://www.cs.cmu.edu/~rbd/papers/cmj-float-to-int.html
    // Might be worth reconsidering in the future.

    // TODO: dither with blue noise?
    float unnorm = std::roundf(sample * 32768.0f);
    return static_cast<short>(std::clamp(unnorm, -32768.0f, 32767.0f));
}

void deinterleave_pcm(const float* raw_buf, int raw_samples, std::vector<std::vector<float>>& channel_bufs) {
    auto samples_per_channel = raw_samples / channel_bufs.size();
    for (auto &channel : channel_bufs) {
        channel.clear();
        channel.resize(samples_per_channel);
    }

    // PCM FORMAT_S16 int -> float32 and de-interleave
    for (auto ch = 0; ch < channel_bufs.size(); ch++) {
        for (auto i = 0; i < samples_per_channel; i++) {
            channel_bufs[ch][i] = raw_buf[i * channel_bufs.size() + ch];
        }
    }
}

void deinterleave_pcm_s16(const short* raw_buf, int raw_samples, std::vector<std::vector<float>>& channel_bufs) {
    auto samples_per_channel = raw_samples / channel_bufs.size();
    for (auto &channel : channel_bufs) {
        channel.clear();
        channel.resize(samples_per_channel);
    }

    // PCM FORMAT_S16 int -> float32 and de-interleave
    for (auto ch = 0; ch < channel_bufs.size(); ch++) {
        for (auto i = 0; i < samples_per_channel; i++) {
            channel_bufs[ch][i] = pcm_s16_to_float32(raw_buf[i * channel_bufs.size() + ch]);
        }
    }
}

}
