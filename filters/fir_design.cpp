#include <span>

#include "fir_design.h"
#include "../log.h"
#include "../util/amplitude.h"
#include "../util/math_ext.h"
#include "../util/window.h"

#include <boost/math/interpolators/makima.hpp>
#include "../external/kissfft/kiss_fftr.h"

namespace fxdsp::fir {

using namespace std::complex_literals;

// Value for freq=0
static constexpr float FREQ_EPSILON = 1e-7;

// Homomorphic out size = (N + 1) / 2, prefers odd N
static constexpr int get_min_phase_size(int size) {
    return size * 2 - 1;
}

static bool filter_to_minimum_phase(std::span<float> linear, std::vector<float>& out) {
    // Mostly a translation of https://github.com/scipy/scipy/blob/v1.7.1/scipy/signal/fir_filter_design.py#L1091-L1263
    auto n_fft = static_cast<int>(pow(2.0f, ceil(log2(2 * (linear.size() - 1) / 0.01))));

    auto cfg = kiss_fft_alloc(n_fft, false, nullptr, nullptr);
    if (cfg == nullptr) {
        return false;
    }

    // Convert to real-only complex and zero-pad
    std::vector<kiss_fft_cpx> time_buf(n_fft);
    for (int i = 0; i < linear.size(); i++) {
        time_buf[i].r = linear[i];
    }

    std::vector<kiss_fft_cpx> freq_buf(n_fft);
    kiss_fft(cfg, time_buf.data(), freq_buf.data());

    // Convert to real-only magnitude
    float min_pos = std::numeric_limits<float>::infinity();
    for (auto& bin : freq_buf) {
        auto mag = sqrt(bin.r * bin.r + bin.i * bin.i);
        bin = { .r = mag };

        // Find min positive for log adjustment
        if (mag < min_pos) {
            min_pos = mag;
        }
    }

    // Log and multiply
    for (auto& bin : freq_buf) {
        bin.r = 0.5f * log(bin.r + (FREQ_EPSILON * min_pos)) / static_cast<float>(n_fft);
    }

    // Back to time domain, real-only
    auto icfg = kiss_fft_alloc(n_fft, true, nullptr, nullptr);
    if (icfg == nullptr) {
        free(cfg);
        return false;
    }
    kiss_fft(icfg, freq_buf.data(), time_buf.data());

    // Homomorphic filter
    std::vector<float> window(n_fft);
    window[0] = 1.0f;
    auto stop = static_cast<int>((linear.size() + 1) / 2);
    std::fill(window.begin() + 1, window.begin() + stop, 2.0f);
    if (linear.size() % 2 == 1) {
        window[stop] = 1.0f;
    }
    for (int i = 0; i < n_fft; i++) {
        time_buf[i] = { .r = time_buf[i].r * window[i], .i = 0.0f };
    }

    // exp in freq domain
    kiss_fft(cfg, time_buf.data(), freq_buf.data());
    for (auto& bin : freq_buf) {
        std::complex<float> cpx(bin.r, bin.i);
        cpx = exp(cpx) / static_cast<float>(n_fft);
        bin = { .r = cpx.real(), .i = cpx.imag() };
    }
    kiss_fft(icfg, freq_buf.data(), time_buf.data());
    free(cfg);
    free(icfg);

    auto out_size = (linear.size() / 2) + (linear.size() % 2);
    if (out.size() < out_size) {
        throw std::out_of_range("Output buffer (" + std::to_string(out.size()) +
                                ") too small for minimum phase filter (" + std::to_string(out_size) +
                                ")");
    }

    // Truncate, real-only
    for (int i = 0; i < out_size; i++) {
        out[i] = time_buf[i].r;
    }

    return true;
}

bool make_filter(std::vector<float>& freqs,
                 std::vector<float>& gains,
                 std::vector<float>& out,
                 float sample_rate,
                 bool minimum_phase) {
    auto n_taps = out.size();
    if (minimum_phase) {
        n_taps = get_min_phase_size(n_taps);
    }

    // Scale freqs if necessary
    auto nyquist = sample_rate / 2.0f;
    if (abs(nyquist - 1.0f) > 0.01f) {
        for (auto& freq : freqs) {
            freq /= nyquist;
        }
    }
    // Always use normalized freqs internally
    nyquist = 1.0f;

    // Add freqs ~0 (to avoid breaking log) and Nyquist, both 0 dB gain
    freqs.insert(freqs.begin(), FREQ_EPSILON);
    freqs.push_back(nyquist);
    gains.insert(gains.begin(), 0.0f);
    gains.push_back(0.0f);

    // Interpolate in log frequency domain
    for (auto& freq : freqs) {
        freq = log2(freq);
    }
    boost::math::interpolators::makima spline(std::move(freqs), std::move(gains));
    auto step = nyquist / static_cast<float>(n_taps - 1);
    std::vector<float> linear_freqs(n_taps);
    std::vector<float> linear_gains(n_taps);
    for (auto i = 0; i < n_taps; i++) {
        // Replace 0 with epsilon to avoid breaking log
        auto freq = log2(i == 0 ? FREQ_EPSILON : (step * i / nyquist));
        // Interpolate dB gain
        auto gain = spline(freq);

        // Homomorphic minimum phase conversion ~= sqrt(gain)
        if (minimum_phase) {
            // dB * 2 = linear ** 2
            gain *= 2;
        }

        // Gain: dB to linear
        gain = amplitude::db_to_linear(gain);
        // Freq: log2 to linear
        freq = pow(2.0f, freq);

        linear_freqs[i] = freq;
        linear_gains[i] = gain;
    }
    // Revert epsilon change
    linear_freqs[0] = 0.0f;

    // Interpolate in linear frequency domain and create FFT coefficients
    spline = boost::math::interpolators::makima(std::move(linear_freqs), std::move(linear_gains));
    std::vector<kiss_fft_cpx> freq_taps(n_taps);
    // Linear phase term to avoid non-casual filter
    auto freq_coeff = -static_cast<float>(n_taps - 1) / 2.0f * 1.0if * PI / nyquist;
    for (auto i = 0; i < n_taps; i++) {
        auto freq = step * i;
        auto gain = spline(freq);

        auto coeff = exp(freq_coeff * freq) * gain;

        // Scale for IFFT
        coeff /= static_cast<float>(n_taps) * 2;
        // std::complex -> kiss_fft_cpx
        // TODO: remove after switching to pocketfft
        freq_taps[i] = { .r = coeff.real(), .i = coeff.imag() };
    }

    // Inverse real-only FFT
    auto cfg = kiss_fftr_alloc(static_cast<int>(n_taps * 2), true, nullptr, nullptr);
    if (cfg == nullptr) {
        return false;
    }
    // Allocate temporary buffer to accommodate results: real IFFT size = (complex coefficients)*2
    std::vector<float> ifft_buf(n_taps * 2);
    kiss_fftri(cfg, freq_taps.data(), ifft_buf.data());
    free(cfg);

    // Window first N coefficients for truncation
    for (auto i = 0; i < n_taps; i++) {
        // TODO: off by one?
        ifft_buf[i] *= window::hann(n_taps, static_cast<float>(i + 1));
    }

    if (minimum_phase) {
        std::span<float> linear(ifft_buf.data(), n_taps);
        return filter_to_minimum_phase(linear, out);
    }

    // Truncate and copy to output
    std::copy(ifft_buf.begin(), ifft_buf.begin() + n_taps, out.begin());

    return true;
}

}
