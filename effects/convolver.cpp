#include "convolver.h"
#include "../util/fft.h"
#include "../util/math_ext.h"

#include "../external/kissfft/kiss_fftr.h"
#include "../external/kissfft/_kiss_fft_guts.h"

namespace fxdsp {

ConvolverEffect::ConvolverEffect(const DSP& dsp, int block_size) :
        Effect(dsp),
        block_size(block_size),
        conv_size(0),
        fft_size(0),
        fft_bins(0),
        channels(dsp.channels),
        channel_spans(channels),
        block_pos(0),
        fft_cfg(nullptr) {
    channel_bufs.resize(dsp.channels);

    for (auto& channel : channel_bufs) {
        channel.resize(block_size);
    }
}

void ConvolverEffect::write_audio(std::vector<std::vector<float>>& buf) {
    // Convert to spans
    for (int i = 0; i < buf.size(); i++) {
        channel_spans[i] = buf[i];
    }

    while (!channel_spans[0].empty()) {
        // Fill block up to block size
        auto block_remaining = block_size - block_pos;
        auto in_remaining = static_cast<int>(channel_spans[0].size());
        auto remaining = std::min(in_remaining, block_remaining);

        // Process each channel
        for (int ch = 0; ch < channel_bufs.size(); ch++) {
            auto& block_buf = channel_bufs[ch];
            auto& in_span = channel_spans[ch];

            // Copy to block
            std::copy(in_span.begin(), in_span.begin() + remaining, block_buf.begin() + block_pos);
        }

        // Update state
        for (int ch = 0; ch < buf.size(); ch++) {
            channel_spans[ch] = channel_spans[ch].subspan(remaining);
        }
        block_pos += remaining;

        // Process full FFT buffer
        if (block_pos == block_size) {
            for (auto& ch : channel_bufs) {
                process_fft_chunk(ch);
            }

            sink->write_audio(channel_bufs);
            block_pos = 0;
        }
    }
}

void ConvolverEffect::reset() {
    Effect::reset();

    // Keep filter, just reset input state
    block_pos = 0;
    channel_spans.clear();
    channel_spans.resize(channels);
    for (auto& buf : channel_bufs) {
        std::fill(buf.begin(), buf.end(), 0.0f);
    }
    std::fill(last_overlap.begin(), last_overlap.end(), 0.0f);
}

void ConvolverEffect::finalize() {
    Effect::finalize();

    // Finish last zero-padded block
    std::vector<std::vector<float>> final_bufs(channels);
    for (int ch = 0; ch < channels; ch++) {
        auto& buf = channel_bufs[ch];
        std::fill(buf.begin() + block_pos, buf.end(), 0.0f);
        process_fft_chunk(buf);

        // Max tail length = M; the rest is undefined
        // TODO: span
        final_bufs[ch] = std::vector<float>(buf.begin(), buf.begin() + block_pos + static_cast<int>(fir_filter_time.size()));
    }
    sink->write_audio(final_bufs);
}

void ConvolverEffect::process_fft_chunk(std::vector<float>& block_buf) {
    // Copy and zero-pad to avoid circular convolution and improve performance
    // (fft_time_buf has static size of fft_size)
    std::copy(block_buf.begin(), block_buf.end(), fft_time_buf.begin());
    std::fill(fft_time_buf.begin() + block_buf.size(), fft_time_buf.end(), 0.0f);

    // Forward FFT
    kiss_fftr(fft_cfg.get(), fft_time_buf.data(), fft_freq_buf.data());

    // Convolve by multiplying complex numbers
    for (auto i = 0; i < fft_bins; i++) {
        kiss_fft_cpx cpx;
        // Multiply with filter FR
        C_MUL(cpx, fft_freq_buf[i], fir_filter_freq[i]);
        fft_freq_buf[i] = cpx;
    }

    // Inverse FFT
    kiss_fftri(ifft_cfg.get(), fft_freq_buf.data(), fft_time_buf.data());

    // Add last overlapping region
    for (auto i = 0; i < last_overlap.size(); i++) {
        fft_time_buf[i] += last_overlap[i];
    }

    // Save new overlap region
    auto tail_start = fft_time_buf.begin() + block_size;
    std::copy(tail_start, tail_start + static_cast<int>(last_overlap.size()), last_overlap.begin());

    // Write non-ringing portion back to block buffer (not yet written to sink)
    std::copy(fft_time_buf.begin(), fft_time_buf.begin() + block_size, block_buf.begin());
}

void ConvolverEffect::set_filter(const std::vector<float>& filter) {
    // Min size = N+M to accommodate ringing tail from linear convolution and avoid circular
    // convolution (tail wrapping around)
    // N+M-1 still results in circular convolution sometimes! e.g. Kronecker delta (filter = [1.0])
    auto tail_size = static_cast<int>(filter.size());
    auto overlap_size = block_size;
    conv_size = block_size + tail_size;

    // Min size to run FFT quickly
    fft_size = next_fft_size(conv_size);
    fft_bins = fft_size / 2 + 1;
    fft_time_buf.resize(fft_size);
    fft_freq_buf.resize(fft_size);

    // Buffer for last overlapping region, zero-initialized
    last_overlap.clear();
    last_overlap.resize(overlap_size);

    // Copy and save *original, unpadded* time domain filter (kept for block convolution)
    fir_filter_time = filter;

    // Init real-only FFT
    fft_cfg = std::unique_ptr<struct kiss_fftr_state>(kiss_fftr_alloc(fft_size, false, nullptr, nullptr));
    ifft_cfg = std::unique_ptr<struct kiss_fftr_state>(kiss_fftr_alloc(fft_size, true, nullptr, nullptr));

    // Copy and zero-pad
    std::copy(fir_filter_time.begin(), fir_filter_time.end(), fft_time_buf.begin());
    std::fill(fft_time_buf.begin() + tail_size, fft_time_buf.end(), 0.0f);

    // Compute complex FFT and save as frequency domain filter
    fir_filter_freq.clear();
    fir_filter_freq.resize(fft_size);
    kiss_fftr(fft_cfg.get(), fft_time_buf.data(), fir_filter_freq.data());

    // Pre-multiply IFFT amplitude scale factor into the filter
    for (auto& cpx : fir_filter_freq) {
        cpx.r /= static_cast<float>(fft_size);
        cpx.i /= static_cast<float>(fft_size);
    }
}

const std::vector<float>& ConvolverEffect::get_filter() {
    return fir_filter_time;
}

}
