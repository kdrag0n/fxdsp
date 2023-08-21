#pragma once

#include <random>

#include "../dsp.h"

namespace fxdsp {

class NoiseEffect : public Effect {
private:
    std::random_device rand_device;
    std::default_random_engine rand_engine;
    std::uniform_real_distribution<float> rand_dist;

public:
    NoiseEffect(const DSP& dsp);
    void write_audio(std::vector<std::vector<float>>& buf) override;
};

}
