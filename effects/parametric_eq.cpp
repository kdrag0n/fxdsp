#include "parametric_eq.h"

namespace fxdsp {

ParametricEqEffect::ParametricEqEffect(const DSP& dsp) :
        Effect(dsp),
        sample_rate(dsp.sample_rate) {
    channel_filters.resize(dsp.channels);
}

void ParametricEqEffect::write_audio(std::vector<std::vector<float>>& buf) {
    for (auto ch = 0; ch < buf.size(); ch++) {
        for (auto& sample : buf[ch]) {
            for (const auto& filter : channel_filters[ch]) {
                sample = filter->process_sample(sample);
            }
        }
    }

    sink->write_audio(buf);
}

unsigned int ParametricEqEffect::add_filter(BiquadFilterType type, float center_freq, float q, float gain_db) {
    unsigned int idx = -1;
    for (auto& filters : channel_filters) {
        filters.push_back(std::make_unique<BiquadFilter>(type, sample_rate, center_freq, q, gain_db));
        idx = filters.size() - 1;
    }

    return idx;
}

void ParametricEqEffect::update_filter(int idx, BiquadFilterType type, float center_freq, float q,
                                       float gain_db) {
    for (auto& filters : channel_filters) {
        filters[idx] = std::make_unique<BiquadFilter>(type, sample_rate, center_freq, q, gain_db);
    }
}

void ParametricEqEffect::remove_filter(int idx) {
    for (auto& filters : channel_filters) {
        filters.erase(filters.begin() + idx);
    }
}

void ParametricEqEffect::remove_all_filters() {
    for (auto& filters : channel_filters) {
        filters.clear();
    }
}

void fxdsp::ParametricEqEffect::reserve_filters(int count) {
    for (auto& filters : channel_filters) {
        filters.reserve(count);
    }
}

void ParametricEqEffect::reset() {
    Effect::reset();

    for (auto& filters : channel_filters) {
        for (auto& filter : filters) {
            filter.reset();
        }
    }
}

}
