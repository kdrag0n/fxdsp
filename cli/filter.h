#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <cstring>
#include <variant>
#include <memory>

#include "../dsp.h"
#include "../wave.h"
#include "../effects/convolver.h"
#include "../effects/gain.h"
#include "../effects/graphic_eq_fir.h"
#include "../effects/parametric_eq.h"
#include "../sinks/collecting_float.h"
#include "../sinks/collecting_s16.h"

using namespace fxdsp;

static constexpr auto TEST_AUTOEQ = false;
static constexpr auto TEST_PEQ = false;
static constexpr auto TEST_GEQ_FIR = true;
static constexpr auto TEST_CONV = false;

static constexpr auto GEQ_FIR_EXPORT_FILTER = true;
static constexpr auto GEQ_FIR_EXPORT_FILTER_PATH = "fir_cpp.wav";

static constexpr auto CONVOLVER_BLOCK_SIZE = 2999;

static std::vector<float> GEQ_BANDS{5.0f, -7.0f, 1.0f, 8.0f, 9.0f, -9.0f, -6.5f, -4.0f, 4.0f, 6.0f};

static bool is_first_init = true;

void process_buffer_short(DSP& dsp, CollectingS16BufferSink& sink, std::vector<short>& buf,
                          const std::vector<std::vector<float>>& fir_filter);
void process_buffer_float(DSP& dsp, CollectingFloatBufferSink& sink, std::vector<float>& buf,
                          const std::vector<std::vector<float>>& fir_filter);

void init_effects(DSP& dsp, const std::vector<std::vector<float>>& fir_filter) {
    // https://github.com/jaakkopasanen/AutoEq/tree/master/results/rtings/rtings_harman_over-ear_2018/HyperX%20Cloud%20II
    if (TEST_AUTOEQ) {
        auto autoEqGain = new GainEffect(dsp, -6.5f);
        auto autoEq = new ParametricEqEffect(dsp);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 48.0f, 1.09f, -2.5f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 134.0f, 1.1f, -4.9f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 997.0f, 1.15f, 2.1f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 3765.0f, 5.12f, 6.1f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 19743.0f, 0.22f, -7.4f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 2348.0f, 2.83f, -2.1f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 3166.0f, 0.65f, 1.0f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 5591.0f, 2.83f, -2.0f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 7842.0f, 1.95f, -2.0f);
        autoEq->add_filter(BIQUAD_PEAKING_EQ, 10073.0f, 1.03f, 1.3f);
        dsp.add_effect(autoEqGain);
        dsp.add_effect(autoEq);
    }

    if (TEST_PEQ) {
        auto eq = new ParametricEqEffect(dsp);
        eq->add_filter(BIQUAD_PEAKING_EQ, 20.0f, 5.0, 5.0f);
        eq->add_filter(BIQUAD_PEAKING_EQ, 10000.0f, 5.0f, 10.0f);
        dsp.add_effect(eq);
    }

    if (TEST_GEQ_FIR) {
        auto geq = new FirGraphicEqEffect(dsp, 10);
        geq->set_all_bands(GEQ_BANDS);
        dsp.add_effect(geq);

        if (GEQ_FIR_EXPORT_FILTER && is_first_init) {
            WaveHeader wave(FORMAT_F32, 1, dsp.sample_rate, geq->get_filter().size());

            std::string out_path(GEQ_FIR_EXPORT_FILTER_PATH);
            std::ofstream out_file(out_path, std::ios::binary);
            out_file.write(reinterpret_cast<char*>(&wave), sizeof(wave));
            out_file.write(reinterpret_cast<const char*>(geq->get_filter().data()), wave.data.size);
            out_file.close();
        }
    }

    if (TEST_CONV && !fir_filter.empty()) {
        auto convolver = new ConvolverEffect(dsp, CONVOLVER_BLOCK_SIZE);
        // Only use first channel for now
        convolver->set_filter(fir_filter[0]);
        dsp.add_effect(convolver);
    }

    is_first_init = false;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [in.wav] [out.wav] {fir_filter.wav}\n";
        return 1;
    }

    // TODO: migrate to common I/O helpers once helpers can do output while preserving format
    std::string in_path(argv[1]);
    std::ifstream in_file(in_path, std::ios::binary);
    std::vector<char> in_buf((std::istreambuf_iterator<char>(in_file)),
                             std::istreambuf_iterator<char>());
    if (in_buf.size() < sizeof(WaveHeader)) {
        std::cerr << "File too small\n";
        return 2;
    }
    in_file.close();

    std::vector<std::vector<float>> fir_filter;
    if (argc >= 4) {
        std::string fir_path(argv[3]);
        auto fir_opt = load_wave_file_float(fir_path);
        fir_filter = std::move(fir_opt);
    }

    std::span<std::byte> in_payload(*reinterpret_cast<std::vector<std::byte>*>(&in_buf));
    WaveHeader wave(in_payload);

    auto num_channels = wave.fmt.num_channels;
    auto num_samples = wave.data.size / (wave.fmt.bits_per_sample / 8);
    auto samples_per_channel = wave.data.size / wave.fmt.block_align;
    auto format = wave.getAudioFormat();

    DSP dsp(format, wave.fmt.sample_rate, num_channels, nullptr);

    // Ugly, but this avoids having to make a copy with malloc
    std::variant<std::shared_ptr<std::vector<short>>, std::shared_ptr<std::vector<float>>> pcm_buf;
    char* pcm_out_data;
    if (format == FORMAT_S16) {
        auto pcm_data = reinterpret_cast<short*>(in_payload.data());
        pcm_buf = std::make_shared<std::vector<short>>(pcm_data, pcm_data + num_samples);

        auto short_buf = std::get<std::shared_ptr<std::vector<short>>>(pcm_buf);
        auto sink = std::make_shared<CollectingS16BufferSink>(num_channels, samples_per_channel);
        dsp.set_sink(sink.get());
        process_buffer_short(dsp, *sink, *short_buf, fir_filter);
        pcm_out_data = reinterpret_cast<char*>(short_buf->data());
    } else if (format == FORMAT_F32) {
        auto pcm_data = reinterpret_cast<float*>(in_payload.data());
        pcm_buf = std::make_shared<std::vector<float>>(pcm_data, pcm_data + num_samples);

        auto float_buf = std::get<std::shared_ptr<std::vector<float>>>(pcm_buf);
        auto sink = std::make_shared<CollectingFloatBufferSink>(num_channels, samples_per_channel);
        dsp.set_sink(sink.get());
        process_buffer_float(dsp, *sink, *float_buf, fir_filter);
        pcm_out_data = reinterpret_cast<char*>(float_buf->data());
    } else {
        std::cerr << "Unknown audio format\n";
        return 3;
    }

    std::string out_path(argv[2]);
    std::ofstream out_file(out_path, std::ios::binary);
    out_file.write(reinterpret_cast<char*>(&wave), sizeof(wave));
    out_file.write(pcm_out_data, wave.data.size);
    out_file.close();

    return 0;
}
