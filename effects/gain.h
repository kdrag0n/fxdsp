#pragma once

#include "../dsp.h"
#include "../util/amplitude.h"

namespace fxdsp {

class GainEffect : public Effect {
private:
    float sample_factor;

public:
    GainEffect(const DSP& dsp, float gain_db);
    void write_audio(std::vector<std::vector<float>>& buf) override;
};

}
