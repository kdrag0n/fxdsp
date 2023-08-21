#include "noise.h"

namespace fxdsp {

NoiseEffect::NoiseEffect(const DSP &dsp) :
        Effect(dsp),
        rand_engine(rand_device()),
        rand_dist(-1.0f, 1.0f) {
}

void NoiseEffect::write_audio(std::vector<std::vector<float>>& buf) {
    for (auto& channel : buf) {
        for (auto& sample : channel) {
            sample = rand_dist(rand_engine);
        }
    }

    sink->write_audio(buf);
}

}
