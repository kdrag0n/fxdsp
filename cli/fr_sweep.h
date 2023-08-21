#pragma once

#include <cmath>

static constexpr float AMPLITUDE = 0.5f;
static constexpr float PHASE = 0.0f;
static constexpr int SAMPLE_RATE = 44100;

static constexpr int MIN_FREQ = 5;
static constexpr int MAX_FREQ = SAMPLE_RATE / 2; // up to Nyquist
static constexpr int FREQ_STEP = MIN_FREQ;

static constexpr int PERIOD = SAMPLE_RATE / FREQ_STEP;

static inline constexpr int get_freq_sample_count(int freq) {
    return PERIOD;
    //return static_cast<int>(std::ceil(SAMPLE_RATE / static_cast<float>(freq) * 3));
}
