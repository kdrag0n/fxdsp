#pragma once

#include <cstdint>

#include "../sink.h"

namespace fxdsp {

class CollectingS16BufferSink : public AudioSink {
private:
    std::vector<uint16_t> out_buf;

public:
    CollectingS16BufferSink(int channels, int samples_per_channel = 0);

    void write_audio(std::vector<std::vector<float>>& buf) override;
    std::vector<uint16_t>& get_buffer();
};

}
