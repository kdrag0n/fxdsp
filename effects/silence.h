#pragma once

#include "../dsp.h"

namespace fxdsp {

class SilenceEffect : public Effect {
public:
    SilenceEffect(const DSP& dsp);
    void write_audio(std::vector<std::vector<float>>& buf) override;
};

}
