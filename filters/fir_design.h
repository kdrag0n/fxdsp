#pragma once

#include <vector>
#include <complex>

namespace fxdsp::fir {

// Number of taps = out.size()
// Only supports type I filters (odd #taps, symmetric)
// freqs, gains are interpolated with modified Akima interpolation
// freqs will be mutated
// gains should be in dB
// This will allocate memory for temporary storage
bool make_filter(std::vector<float> &freqs,
                 std::vector<float> &gains,
                 std::vector<float> &out,
                 float sample_rate = 2.0f,
                 bool minimum_phase = true);

}
