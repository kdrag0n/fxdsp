#include "biquad.h"
#include "../log.h"
#include "../util/math_ext.h"
#include "../util/amplitude.h"

#include <cmath>
#include <complex>

using namespace std::complex_literals;

namespace fxdsp {

static constexpr auto GRAPH_MIN_FREQ = 0.000001f;

// https://www.w3.org/TR/audio-eq-cookbook/
BiquadFilter::BiquadFilter(BiquadFilterType type, float sample_rate, float center_freq, float q,
                           float gain_db) : x1(0.0f), y1(0.0f), x2(0.0f), y2(0.0f) {
    // TODO: fix double conversions
    float A = (type >= BIQUAD_PEAKING_EQ)
              ? pow(10.0f, gain_db / 40.0f)
              : nan;
    float w0 = 2.0f * M_PI * (center_freq / sample_rate);
    float cosW0 = cos(w0);
    float sinW0 = sin(w0);
    float a = sinW0 / (2.0f * q);

    float a0, a1, a2, b0, b1, b2;
    switch (type) {
        case BIQUAD_LOW_PASS:
            b0 = (1.0f - cosW0) / 2.0f;
            b1 = 1.0f - cosW0;
            b2 = (1.0f - cosW0) / 2.0f;
            a0 = 1.0f + a;
            a1 = -2.0f * cosW0;
            a2 = 1.0f - a;
            break;
        case BIQUAD_HIGH_PASS:
            b0 = (1.0f + cosW0) / 2.0f;
            b1 = -(1.0f + cosW0);
            b2 = (1.0f + cosW0) / 2.0f;
            a0 = 1.0f + a;
            a1 = -2.0f * cosW0;
            a2 = 1.0f - a;
            break;
        case BIQUAD_BAND_PASS_PEAK_Q:
            b0 = q * a;
            b1 = 0.0f;
            b2 = -q * a;
            a0 = 1.0f + a;
            a1 = -2.0f * cosW0;
            a2 = 1.0f - a;
            break;
        case BIQUAD_BAND_PASS_PEAK_0:
            b0 = a;
            b1 = 0.0f;
            b2 = -a;
            a0 = 1.0f + a;
            a1 = -2.0f * cosW0;
            a2 = 1.0f - a;
            break;
        case BIQUAD_NOTCH:
            b0 = 1.0f;
            b1 = -2.0f * cosW0;
            b2 = 1.0f;
            a0 = 1.0f + a;
            a1 = -2.0f * cosW0;
            a2 = 1.0f - a;
            break;
        case BIQUAD_ALL_PASS:
            b0 = 1.0f - a;
            b1 = -2.0f * cosW0;
            b2 = 1.0f + a;
            a0 = 1.0f + a;
            a1 = -2.0f * cosW0;
            a2 = 1.0f - a;
            break;
        case BIQUAD_PEAKING_EQ:
            b0 = 1.0f + a * A;
            b1 = -2.0f * cosW0;
            b2 = 1.0f - a * A;
            a0 = 1.0f + a / A;
            a1 = -2.0f * cosW0;
            a2 = 1.0f - a / A;
            break;
        case BIQUAD_LOW_SHELF:
            b0 = A * ((A+1.0f) - (A-1.0f)*cosW0 + 2.0f*sqrt(A)*a);
            b1 = 2.0f*A * ((A-1.0f) - (A+1.0f)*cosW0);
            b2 = A * ((A+1.0f) - (A-1.0f)*cosW0 - 2.0f*sqrt(A)*a);
            a0 = (A+1.0f) + (A-1.0f)*cosW0 + 2.0f*sqrt(A)*a;
            a1 = -2.0f * ((A-1.0f) + (A+1.0f)*cosW0);
            a2 = (A+1.0f) + (A-1.0f)*cosW0 - 2.0f*sqrt(A)*a;
            break;
        case BIQUAD_HIGH_SHELF:
            b0 = A * ((A+1.0f) + (A-1.0f)*cosW0 + 2.0f*sqrt(A)*a);
            b1 = -2.0f*A * ((A-1.0f) + (A+1.0f)*cosW0);
            b2 = A * ((A+1.0f) + (A-1.0f)*cosW0 - 2.0f*sqrt(A)*a);
            a0 = (A+1.0f) - (A-1.0f)*cosW0 + 2.0f*sqrt(A)*a;
            a1 = 2.0f * ((A-1.0f) - (A+1.0f)*cosW0);
            a2 = (A+1.0f) - (A-1.0f)*cosW0 - 2.0f*sqrt(A)*a;
            break;
        default:
            a0 = a1 = a2 = b0 = b1 = b2 = nan;
            break;
    }

    // Pre-compute normalized coefficients
    b0_a0 = b0 / a0;
    b1_a0 = b1 / a0;
    b2_a0 = b2 / a0;
    a1_a0 = a1 / a0;
    a2_a0 = a2 / a0;

    ALOGV("biquad: a1=%f a2=%f b0=%f b1=%f b2=%f\n", a1_a0, a2_a0, b0_a0, b1_a0, b2_a0);
}

float BiquadFilter::process_sample(float sample) {
    // Direct Form 1
    float result = b0_a0 * sample + b1_a0 * x1 + b2_a0 * x2
            - a1_a0 * y1 - a2_a0 * y2;

    // Shift state
    x2 = x1;
    y2 = y1;
    x1 = sample;
    y1 = result;

    return result;
}

void BiquadFilter::reset() {
    x1 = y1 = x2 = y2 = 0.0f;
}

void BiquadFilter::gen_graph(std::vector<float> &out_x, std::vector<float> &out_y, float max_freq) const {
    auto count = out_y.size();
    auto log_min = log2(20.f);
    auto log_max = log2(max_freq);
    auto log_step = (log_max - log_min) / static_cast<float>(count);

    // https://dsp.stackexchange.com/a/16888
    for (int i = 0; i < count; i++) {
        auto w = pow(2.0f, log_min + log_step * static_cast<float>(i)) / max_freq * static_cast<float>(M_PI);

        // Exponent
        auto ze = exp(w * -1.0if);
        // Transfer function
        auto H = (b0_a0 * ze*ze + b1_a0 * ze + b2_a0) /
                (/*1*/ ze*ze + a1_a0 * ze + a2_a0);
        auto Ha = abs(H);
        auto Hdb = amplitude::linear_to_db(Ha);
        auto freq = w / static_cast<float>(M_PI) * max_freq;

        // Normalize x for log scale
        out_x[i] = (log2(freq) - log_min) / (log_max - log_min) * max_freq;
        out_y[i] = Hdb;
    }
}

// http://www.sengpielaudio.com/calculator-bandwidth.htm
float octave_bw_to_q(float n) {
    float pow_n = pow(2.0f, n);
    return sqrt(pow_n) / (pow_n - 1.0f);
}

float bw_to_q(float sample_rate, float center_freq, float bw) {
    float w0 = 2.0f * M_PI * (center_freq / sample_rate);
    return 1.0f / (2.0f * sinh((M_LN2 / 2.0f) * bw * (w0 / sin(w0))));
}

float s_to_q(float gain_db, float s) {
    float A = pow(10.0f, gain_db / 40.0f);
    return 1.0f / sqrt((A + (1.0f / A)) * ((1.0f / s) - 1) + 2.0f);
}

}
