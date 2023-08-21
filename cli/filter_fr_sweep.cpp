#include "filter.h"
#include "fr_sweep.h"

void process_buffer_short(DSP& dsp, CollectingS16BufferSink& sink, std::vector<short>& buf,
                          const std::vector<std::vector<float>>& fir_filter) {
    // Unsupported
    exit(3);
}

void process_buffer_float(DSP& dsp, CollectingFloatBufferSink& sink, std::vector<float>& buf,
                          const std::vector<std::vector<float>>& fir_filter) {
    init_effects(dsp, fir_filter);

    int si = 0; // total
    for (int freq = MIN_FREQ; freq < MAX_FREQ; freq += FREQ_STEP) {
        int period_samples = get_freq_sample_count(freq);
        std::vector<float> freq_buf(buf.begin() + si, buf.begin() + si + period_samples);

        sink.set_limit(period_samples);
        dsp.write_audio_1d(freq_buf);

        si += period_samples;
        for (auto effect : dsp.get_effects()) {
            effect->finalize();
            effect->reset();
        }
    }

    buf = sink.get_buffer();
}
