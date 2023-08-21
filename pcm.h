#pragma once

#include <vector>

namespace fxdsp {

enum AudioFormat {
    FORMAT_UNKNOWN = 0,
    FORMAT_U8,
    FORMAT_S16,
    FORMAT_S24,
    FORMAT_F32,
};

float pcm_s16_to_float32(short sample);
short pcm_float32_to_s16(float sample);

void deinterleave_pcm(const float* raw_buf, int raw_samples, std::vector<std::vector<float>>& channel_bufs);
void deinterleave_pcm_s16(const short* raw_buf, int raw_samples, std::vector<std::vector<float>>& channel_bufs);

}
