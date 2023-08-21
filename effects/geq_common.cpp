#include <cmath>

#include "geq_common.h"
#include "../log.h"
#include "../filters/biquad.h"

namespace fxdsp {

GraphicEqBand::GraphicEqBand(float center_freq, float q, float gain_db) :
        center_freq(center_freq), q(q), gain_db(gain_db) {
}

GraphicEqBase::GraphicEqBase(const DSP& dsp, int num_bands, float start_freq, float end_freq) :
        Effect(dsp) {
    init_bands(num_bands, start_freq, end_freq);
}

void GraphicEqBase::init_bands(int num_bands, float start_freq, float end_freq) {
    float octave_count = log2(end_freq / start_freq);
    // Calculate Q from octave BW, based on band size in octaves
    float q = octave_bw_to_q(octave_count / static_cast<float>(num_bands));

    bands.reserve(num_bands);
    for (int band_idx = 0; band_idx < num_bands; band_idx++) {
        // log2 because bands are allocated by octaves
        float log_bw = (log2(end_freq) - log2(start_freq)) / static_cast<float>(num_bands);
        float log_start_freq = static_cast<float>(band_idx) * log_bw;
        float log_end_freq = static_cast<float>(band_idx + 1) * log_bw;

        // Log scale -> linear bands + arithmetic mean in log scale
        float band_center = start_freq * pow(2.0f, (log_start_freq + log_end_freq) / 2.0f);
        ALOGV("GEQ init: center=%f  q=%f\n", band_center, q);

        // Start with 0 dB gain
        bands.emplace_back(band_center, q, 0.0f);
    }
}

void GraphicEqBase::set_all_bands(const std::vector<float> &gains) {
    for (auto i = 0; i < bands.size(); i++) {
        bands[i].gain_db = gains[i];
    }

    build_filters();
}

void GraphicEqBase::set_band_gain(int band_idx, float gain_db) {
    bands[band_idx].gain_db = gain_db;
    build_filters();
}

GraphicEqBand &GraphicEqBase::get_band(int band_idx) {
    return bands[band_idx];
}

}
