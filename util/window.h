#pragma once

namespace fxdsp::window {

// Normal symmetric variant for filter design
float hann(float num_samples, float n);
// Periodic variant for DFT circular convolution
float hann_periodic(float num_samples, float n);

}
