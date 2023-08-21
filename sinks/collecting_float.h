#pragma once

#include "../sink.h"

namespace fxdsp {

class CollectingFloatBufferSink : public AudioSink {
private:
    std::vector<float> out_buf;
    size_t limit;

public:
    CollectingFloatBufferSink(int channels, int samples_per_channel = 0);

    void write_audio(std::vector<std::vector<float>>& buf) override;
    std::vector<float>& get_buffer();

    void set_limit(size_t limit);
};

}
