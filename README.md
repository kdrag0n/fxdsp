# fxdsp

Fast and simple C++ DSP engine with high-quality effects.

Originally designed for use in [PhantomAmp](https://www.patreon.com/posts/exclusive-audio-72949048), an Android app for rootless system-wide audio effects.

## Features

- [Parametric equalizer](effects/parametric_eq.cpp) powered by [biquad IIR filters](filters/biquad.cpp)
  - Low-pass, high-pass, band-pass, notch, all-pass, peaking EQ, low shelf, high shelf
  - Frequency response graph generator for GUI
- [FIR graphic equalizer](effects/graphic_eq_fir.cpp) with dynamic [FIR filter design](filters/fir_design.cpp)
  - Supports both linear phase and minimum phase
  - Smooth Makima spline interpolation of gains, without overshoot
  - Intuitive GUI graphs (frequency and phase response) using PCHIP interpolation
- [IIR graphic equalizer](effects/graphic_eq_iir.cpp) using peaking EQ biquad filters
- [Convolver](effects/convolver.cpp) for custom FIR filters (as WAV files)
  - Optimized FFT-based convolution, overlap-add
- Low-latency audio output on Android using [Oboe](sinks/oboe.cpp)
- 32-bit floating point processing, for quality and performance

## Build

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

CLI tools for testing:

- `fxdsp-filter-fr-sweep`
- `fxdsp-filter-test`
- `fxdsp-gen-fr-test-combined`
- `fxdsp-gen-fr-test-sweep`
