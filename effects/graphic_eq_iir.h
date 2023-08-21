#pragma once

#include <vector>

#include "../dsp.h"
#include "../filters/biquad.h"
#include "parametric_eq.h"
#include "geq_common.h"

namespace fxdsp {

class IirGraphicEqEffect : public GraphicEqBase {
private:
    // Underlying biquad container
    ParametricEqEffect peq;

    void build_filters() override;

public:
    IirGraphicEqEffect(const DSP& dsp, int num_bands, float start_freq = 20.0f, float end_freq = 20000.0f);
    void write_audio(std::vector<std::vector<float>>& buf) override;
    void set_next_sink(AudioSink& next_sink) override;
    void reset() override;
};

}
