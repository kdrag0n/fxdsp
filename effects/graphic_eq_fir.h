#pragma once

#include <vector>

#include "../dsp.h"
#include "../filters/biquad.h"
#include "convolver.h"
#include "geq_common.h"

namespace fxdsp {

class FirGraphicEqEffect : public GraphicEqBase {
private:
    // Underlying FIR convolver
    ConvolverEffect convolver;

    // For building FIR filter
    int sample_rate;

    void build_filters() override;

public:
    FirGraphicEqEffect(const DSP& dsp,
                       int num_bands,
                       int block_size = 4999,
                       float start_freq = 20.0f,
                       float end_freq = 20000.0f);

    void write_audio(std::vector<std::vector<float>>& buf) override;
    // Delegate
    void set_next_sink(AudioSink& next_sink) override;
    void reset() override;
    void finalize() override;

    const std::vector<float>& get_filter();
};

}
