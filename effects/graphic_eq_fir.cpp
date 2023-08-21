#include <cmath>

#include "graphic_eq_fir.h"
#include "../filters/fir_design.h"
#include "../log.h"

namespace fxdsp {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "VirtualCallInCtorOrDtor"
FirGraphicEqEffect::FirGraphicEqEffect(const DSP& dsp,
                                       int num_bands,
                                       int block_size,
                                       float start_freq,
                                       float end_freq) :
                                       GraphicEqBase(dsp, num_bands, start_freq, end_freq),
                                       convolver(dsp, block_size),
                                       sample_rate(dsp.sample_rate) {
    // Safe in this context as it delegates to this derived class' implementation
    build_filters();
}
#pragma clang diagnostic pop

void FirGraphicEqEffect::write_audio(std::vector<std::vector<float>>& buf) {
    convolver.write_audio(buf);
}

void FirGraphicEqEffect::set_next_sink(AudioSink &next_sink) {
    Effect::set_next_sink(next_sink);
    convolver.set_next_sink(next_sink);
}

void FirGraphicEqEffect::reset() {
    Effect::reset();
    convolver.reset();
}

void FirGraphicEqEffect::finalize() {
    Effect::finalize();
    convolver.finalize();
}

void FirGraphicEqEffect::build_filters() {
    std::vector<float> freqs;
    freqs.reserve(bands.size());
    std::vector<float> gains;
    gains.reserve(bands.size());

    for (auto& band : bands) {
        ALOGV("GEQ build: center=%f gain=%f\n", band.center_freq, band.gain_db);
        freqs.push_back(band.center_freq);
        gains.push_back(band.gain_db);
    }

    std::vector<float> filter(convolver.block_size);
    auto success = fir::make_filter(freqs, gains, filter, static_cast<float>(sample_rate));
    if (!success) {
        ALOGE("GEQ build: Failed to build FIR filter: %d", success);
        return;
    }

    convolver.set_filter(filter);
}

const std::vector<float>& FirGraphicEqEffect::get_filter() {
    return convolver.get_filter();
}

}
