#include "oboe.h"

namespace fxdsp {

OboeSink::OboeSink(int channels, int session_id) :
        AudioSink(FORMAT_F32, channels),
        frame_size(0) {
    builder.setDirection(oboe::Direction::Output)
        ->setSampleRate(48000)
        ->setPerformanceMode(oboe::PerformanceMode::None)
        ->setSharingMode(oboe::SharingMode::Shared)
        ->setFormat(oboe::AudioFormat::Float)
        ->setChannelCount(channels)
        ->setSessionId(static_cast<oboe::SessionId>(session_id));
}

void OboeSink::open() {
    auto result = builder.openStream(stream);
    if (result != oboe::Result::OK) {
        throw std::runtime_error(oboe::convertToText(result));
    }

    result = stream->requestStart();
    if (result != oboe::Result::OK) {
        throw std::runtime_error(oboe::convertToText(result));
    }

    frame_size = stream->getBytesPerFrame();
    buffer_size = stream->getBufferCapacityInFrames() * stream->getBytesPerFrame();
}

void OboeSink::close() {
    if (stream != nullptr) {
        stream->requestStop();
        stream->close();
    }
}

void OboeSink::write_audio(std::vector<std::vector<float>> &buf) {
    auto samples_per_ch = buf[0].size();
    interleaved_buf.resize(samples_per_ch * channels);
    for (int ch = 0; ch < channels; ch++) {
        auto& ch_buf = buf[ch];
        for (int i = 0; i < samples_per_ch; i++) {
            interleaved_buf[i * channels + ch] = ch_buf[i];
        }
    }

    auto sample_bytes = samples_per_ch * sizeof(buf[0][0]) * channels;
    if (stream != nullptr) {
        stream->write(interleaved_buf.data(), static_cast<int32_t>(sample_bytes) / frame_size, 0);
    }
}

}
