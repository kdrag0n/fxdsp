#include "graph.h"
#include "fft.h"
#include "../log.h"

#include "../external/kissfft/kiss_fftr.h"
#include "amplitude.h"
#include <boost/math/interpolators/pchip.hpp>

namespace fxdsp::graph {

static constexpr auto MIN_FREQ = 20.0f;
static constexpr auto FREQ_EPSILON = 1e-6f;

void interpolate(std::vector<float>& in_x, std::vector<float>& in_y,
                 std::vector<float>& out_x, std::vector<float>& out_y,
                 float start_x) {
    auto count = out_x.size();
    if (isnan(start_x)) {
        start_x = in_x[0];
    }
    auto end_x = in_x[in_x.size() - 1];
    auto x_step = (end_x - start_x) / static_cast<float>(count - 1);

    boost::math::interpolators::pchip spline(std::move(in_x), std::move(in_y));

    for (auto i = 0; i < count; i++) {
        auto x = start_x + x_step * static_cast<float>(i);
        // Avoid crash caused by rounding errors
        auto y = spline(std::min(std::max(x, start_x), end_x));
        out_x[i] = x;
        out_y[i] = y;
    }
}

void impulse_response_curves(std::vector<float>& ir,
                             std::vector<float>& ir_out_x,
                             std::vector<float>& ir_out_y,
                             std::vector<float>& fr_out_x,
                             std::vector<float>& fr_out_y,
                             std::vector<float>& pr_out_x,
                             std::vector<float>& pr_out_y,
                             float max_freq) {
    if (ir.empty()) {
        return;
    }

    // FFT buffers
    auto num_bins = ir.size() / 2;
    auto fft_size = next_fft_size(static_cast<int>(ir.size()));
    std::vector<float> fft_x(num_bins);
    std::vector<kiss_fft_cpx> fft_y(fft_size / 2);
    auto fft_x_max = static_cast<float>(num_bins - 1);
    auto log_min = log2(MIN_FREQ / max_freq);
    auto log_max = log2(1.0f);
    for (int i = 0; i < num_bins; i++) {
        // 0-1 scale for UI graph
        auto freq_frac = i == 0 ? FREQ_EPSILON : static_cast<float>(i) / fft_x_max;
        fft_x[i] = (log2(freq_frac) - log_min) / (log_max - log_min);
    }

    // FFT zero-padding
    std::vector<float> fft_in(fft_size);
    std::copy(ir.begin(),  ir.end(), fft_in.begin());
    auto fft_cfg = kiss_fftr_alloc(fft_size, false, nullptr, nullptr);
    kiss_fftr(fft_cfg, fft_in.data(), fft_y.data());
    free(fft_cfg);

    // FR & PR
    std::vector<float> fr_y(num_bins);
    std::vector<float> pr_y(num_bins);
    for (int i = 0; i < num_bins; i++) {
        auto cpx = fft_y[i];
        fr_y[i] = amplitude::linear_to_db(sqrt(cpx.r*cpx.r + cpx.i*cpx.i));
        pr_y[i] = atan2(cpx.i, cpx.r);
    }

    // Interpolate to match UI and remove negative x part
    // Copy before interpolate for second call to own it
    auto fft_x_copy = fft_x;
    interpolate(fft_x, fr_y, fr_out_x, fr_out_y, 0.0f);
    interpolate(fft_x_copy, pr_y, pr_out_x, pr_out_y, 0.0f);

    // IR
    // This is done after FFT to avoid copying ir, as interpolate needs ownership of it
    std::vector<float> ir_x(ir.size());
    auto x_max = static_cast<float>(ir.size() - 1);
    for (int i = 0; i < ir.size(); i++) {
        // 0-1 scale for UI graph
        ir_x[i] = static_cast<float>(i) / x_max;
    }
    interpolate(ir_x, ir, ir_out_x, ir_out_y);
}

}
