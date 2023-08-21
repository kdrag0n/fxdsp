#include <cmath>

namespace fxdsp::window {

// https://en.wikipedia.org/wiki/Window_function#Hann_and_Hamming_windows
float hann(float num_samples, float n) {
    // Canonical cosine implementation:
    //return 0.5 - 0.5 * cos(M_2_PI * n / (num_samples - 1));

    // Optimized sine-based implementation (less ops)
    float x = sin(static_cast<float>(M_PI) * n / (num_samples - 1));
    return x * x;
}

// Effectively equivalent to calculating a window for N+1 and then truncating it to N
// Specialized for performance
// https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.windows.hann.html#scipy.signal.windows.hann
float hann_periodic(float num_samples, float n) {
    float x = sin(static_cast<float>(M_PI) * n / num_samples);
    return x * x;
}

}
