#include "gain.h"

namespace fxdsp {

GainEffect::GainEffect(const DSP &dsp, float gain_db) :
        Effect(dsp),
        sample_factor(amplitude::db_to_linear(gain_db)) {
}

void GainEffect::write_audio(std::vector<std::vector<float>>& buf) {
    for (auto& channel : buf) {
        for (auto& sample : channel) {
            sample *= sample_factor;
        }
    }

    sink->write_audio(buf);
}

}
