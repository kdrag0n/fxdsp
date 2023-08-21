#include "collecting_s16.h"

namespace fxdsp {

CollectingS16BufferSink::CollectingS16BufferSink(int channels, int samples_per_channel) :
        AudioSink(FORMAT_S16, channels) {
    out_buf.reserve(samples_per_channel * channels);
}

void CollectingS16BufferSink::write_audio(std::vector<std::vector<float>>& buf) {
    auto start_pos = out_buf.size();
    out_buf.resize(start_pos + buf[0].size() * channels);

    // Convert back to S16 int and re-interleave
    for (auto ch = 0; ch < channels; ch++) {
        for (auto i = 0; i < buf[ch].size(); i++) {
            out_buf[start_pos + i * channels + ch] = pcm_float32_to_s16(buf[ch][i]);
        }
    }
}

std::vector<uint16_t> &CollectingS16BufferSink::get_buffer() {
    return out_buf;
}

}
