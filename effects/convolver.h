#pragma once

#include <vector>
#include <span>

#include "../dsp.h"
#include "../util/amplitude.h"

#include "../external/kissfft/kiss_fftr.h"

namespace fxdsp {

class ConvolverEffect : public Effect {
private:
    // Sizes: fft > conv > block
    // Input buffer block size, excl. conv and FFT padding
    int block_size;
    // Size of resulting linear convolution: L+M
    int conv_size;
    // FFT size, incl. conv and FFT padding
    int fft_size;
    // FFT bin count (fft_size/2+1 for real FFT)
    int fft_bins;

    // Allocated to block size, excl. all padding
    std::vector<std::vector<float>> channel_bufs;
    int channels;
    // Allocated to channel count
    std::vector<std::span<float>> channel_spans;
    // Current position in per-channel block buffers
    int block_pos;

    // Only one channel is processed at a time, so only one of each
    // Allocated to FFT size, incl. all padding
    std::vector<float> fft_time_buf;
    std::vector<kiss_fft_cpx> fft_freq_buf;

    // Overlapping region (L) from last block
    std::vector<float> last_overlap;

    // FIR filter in time and frequency domains
    std::vector<float> fir_filter_time; // excl. zero pad
    std::vector<kiss_fft_cpx> fir_filter_freq; // incl. zero pad

    // kissfft configs for forward and inverse FFT
    std::unique_ptr<struct kiss_fftr_state> fft_cfg;
    std::unique_ptr<struct kiss_fftr_state> ifft_cfg;

    // Process the current accumulated input buffer
    void process_fft_chunk(std::vector<float>& block_buf);

    friend class FirGraphicEqEffect;

public:
    ConvolverEffect(const DSP& dsp, int block_size);
    void write_audio(std::vector<std::vector<float>>& buf) override;
    void reset() override;
    // This allocates! Not normally used
    void finalize() override;

    // FIR time domain filter
    void set_filter(const std::vector<float>& filter);
    const std::vector<float>& get_filter();
};

}
