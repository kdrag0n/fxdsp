#include "debug.h"

namespace fxdsp {

static std::string NO_CHAR = "";

// C++ lacks multi-byte Unicode awareness, so do our own indexing
static const std::string WAVEFORM[] = {
    "_",
    "⎽",
    "⎼",
    "—",
    "⎻",
    "⎺",
    "‾",
    "Z",
    "D",
    "C",
    "Y",
};

// Sparkline debugging for audio waveforms
// Translated from: https://github.com/sudara/melatonin_audio_sparklines/blob/main/sparklines.py
std::string sparkline(const std::vector<float>& samples) {
    auto max = *std::max(samples.begin(), samples.end());
    auto scale = (max > 0.0f) ? max : 1.0f;
    auto num_zeros = 0;
    std::string output = "[";
    std::string& last_char = NO_CHAR;

    for (auto i = 0; i < samples.size(); i++) {
        auto sample = samples[i];
        if (sample == 0.0f) {
            if (num_zeros == 0) {
                output += '0';
            }
            num_zeros++;
            continue;
        } else {
            num_zeros = 0;
        }

        if (num_zeros > 1) {
            output += '(' + std::to_string(num_zeros) + ')';
            num_zeros = 0;
        }

        if ((std::abs(sample) - 0.00000015) > 1.0f) {
            // Out of bounds
            output += 'E';
        } else if ((i > 0) && ((sample < 0) != (samples[i-1] < 0))) {
            // Zero crossing
            output += 'x';
        } else {
            // Waveform
            // Normalize for detail
            sample /= scale;
            // Positive 0-6
            auto index = std::min(std::max(static_cast<int>((sample + 1) / 2.0f * 6.99f), 0), 6);
            auto character = WAVEFORM[index];
            // No duplicate chars (test string for Unicode awareness)
            if (last_char != character) {
                output += character;
                last_char = character;
            }
        }
    }

    if (num_zeros > 1) {
        output += '(' + std::to_string(num_zeros) + ')';
    }

    output += ']';
    return output;
}

}
