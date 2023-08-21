#pragma once

#include <vector>
#include <memory>

#include "../dsp.h"
#include "../filters/biquad.h"

namespace fxdsp {

class ParametricEqEffect : public Effect {
private:
    // [channel: [filters]] to keep state separate per channel
    std::vector<std::vector<std::unique_ptr<BiquadFilter>>> channel_filters;
    int sample_rate;

    // Private API for GEQ
    void reserve_filters(int count);
    friend class IirGraphicEqEffect;

public:
    ParametricEqEffect(const DSP& dsp);
    void write_audio(std::vector<std::vector<float>>& buf) override;

    unsigned int add_filter(BiquadFilterType type, float center_freq, float q,
                            float gain_db = std::numeric_limits<double>::quiet_NaN());
    void update_filter(int idx, BiquadFilterType type, float center_freq, float q,
                       float gain_db = std::numeric_limits<double>::quiet_NaN());
    void remove_filter(int idx);
    void remove_all_filters();

    void reset() override;
};

}
