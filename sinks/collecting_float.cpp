#include "collecting_float.h"

namespace fxdsp {

CollectingFloatBufferSink::CollectingFloatBufferSink(int channels, int samples_per_channel) :
        AudioSink(FORMAT_F32, channels),
        limit(SIZE_MAX) {
    out_buf.reserve(samples_per_channel * channels);
}

void CollectingFloatBufferSink::write_audio(std::vector<std::vector<float>>& buf) {
    auto start_pos = out_buf.size();
    auto new_spc = std::min(buf[0].size(), limit);
    out_buf.resize(start_pos + new_spc * channels);
    limit -= new_spc;

    // Convert back to float32 and re-interleave
    for (auto ch = 0; ch < channels; ch++) {
        for (auto i = 0; i < new_spc; i++) {
            out_buf[start_pos + i * channels + ch] = buf[ch][i];
        }
    }
}

std::vector<float>& CollectingFloatBufferSink::get_buffer() {
    return out_buf;
}

void CollectingFloatBufferSink::set_limit(size_t limit) {
    this->limit = limit;
}

}
