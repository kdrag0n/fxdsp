#include "filter.h"

void process_buffer_short(DSP& dsp, CollectingS16BufferSink& sink, std::vector<short>& buf,
                          const std::vector<std::vector<float>>& fir_filter) {
    init_effects(dsp, fir_filter);
    dsp.write_audio_1d(buf);
    std::copy(sink.get_buffer().begin(), sink.get_buffer().end(), buf.begin());
}

void process_buffer_float(DSP& dsp, CollectingFloatBufferSink& sink, std::vector<float>& buf,
                          const std::vector<std::vector<float>>& fir_filter) {
    init_effects(dsp, fir_filter);
    dsp.write_audio_1d(buf);
    std::copy(sink.get_buffer().begin(), sink.get_buffer().end(), buf.begin());
}
