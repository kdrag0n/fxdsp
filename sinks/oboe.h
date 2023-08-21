#pragma once

#include <oboe/Oboe.h>

#include "../sink.h"

namespace fxdsp {

class OboeSink : public AudioSink {
private:
    std::shared_ptr<oboe::AudioStream> stream;
    oboe::AudioStreamBuilder builder;

    // TODO: never allocate
    std::vector<float> interleaved_buf;
    int32_t frame_size;

public:
    OboeSink(int channels, int session_id);

    int32_t buffer_size;

    void write_audio(std::vector<std::vector<float>>& buf) override;
    void open();
    void close();
};

}
