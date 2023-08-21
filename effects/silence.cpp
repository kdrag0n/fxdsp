#include "silence.h"

namespace fxdsp {

SilenceEffect::SilenceEffect(const DSP &dsp) :
        Effect(dsp) {
}

void SilenceEffect::write_audio(std::vector<std::vector<float>>& buf) {
    for (auto& channel : buf) {
        std::fill(channel.begin(), channel.end(), 0);
    }

    sink->write_audio(buf);
}

}
