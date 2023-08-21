#include <cmath>

#include "graphic_eq_iir.h"
#include "../log.h"

namespace fxdsp {

IirGraphicEqEffect::IirGraphicEqEffect(const DSP& dsp, int num_bands, float start_freq, float end_freq) :
        GraphicEqBase(dsp, num_bands, start_freq, end_freq),
        peq(dsp) {
}

void IirGraphicEqEffect::write_audio(std::vector<std::vector<float>>& buf) {
    peq.write_audio(buf);
}

void IirGraphicEqEffect::set_next_sink(AudioSink &next_sink) {
    Effect::set_next_sink(next_sink);
    peq.set_next_sink(next_sink);
}

void IirGraphicEqEffect::build_filters() {
    peq.remove_all_filters();
    peq.reserve_filters(bands.size());

    for (auto& band : bands) {
        ALOGV("GEQ build: center=%f q=%f  gain=%f\n", band.center_freq, band.q, band.gain_db);
        peq.add_filter(BIQUAD_PEAKING_EQ, band.center_freq, band.q, band.gain_db);
    }
}

void IirGraphicEqEffect::reset() {
    Effect::reset();
    peq.reset();
}

}
