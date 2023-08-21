#pragma once

#include <limits>
#include <vector>

namespace fxdsp {

enum BiquadFilterType {
    BIQUAD_LOW_PASS,
    BIQUAD_HIGH_PASS,
    BIQUAD_BAND_PASS_PEAK_Q,
    BIQUAD_BAND_PASS_PEAK_0,
    BIQUAD_NOTCH,
    BIQUAD_ALL_PASS,

    // Order is important - guards A calculation
    BIQUAD_PEAKING_EQ,
    BIQUAD_LOW_SHELF,
    BIQUAD_HIGH_SHELF,
};

class BiquadFilter {
private:
    // Final coefficients for evaluation
    float b0_a0;
    float b1_a0;
    float b2_a0;
    float a1_a0;
    float a2_a0;

    // IIR state
    float x1, y1;
    float x2, y2;

public:
    BiquadFilter(BiquadFilterType type, float sample_rate, float center_freq, float q,
                 float gain_db = std::numeric_limits<double>::quiet_NaN());

    float process_sample(float sample);
    void reset();

    // UI
    void gen_graph(std::vector<float> &out_x, std::vector<float> &out_y, float max_freq) const;
};

float octave_bw_to_q(float n);

float bw_to_q(float sample_rate, float center_freq, float bw);

float s_to_q(float gain_db, float s);

}
