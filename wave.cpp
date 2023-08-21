#include "wave.h"

#include <cstring>
#include <exception>

namespace fxdsp {

WaveHeader::WaveHeader(AudioFormat dsp_format, uint16_t channels, uint32_t sample_rate,
                       size_t samples_per_channel) :
                       riff{}, fmt{}, data{} {
    // IDs
    memcpy(riff.id, CHUNK_ID_RIFF, CHUNK_ID_SIZE);
    memcpy(riff.format, RIFF_FORMAT_WAVE, CHUNK_ID_SIZE);
    memcpy(fmt.id, CHUNK_ID_FMT, CHUNK_ID_SIZE);
    memcpy(data.id, CHUNK_ID_DATA, CHUNK_ID_SIZE);

    // Format
    fmt.num_channels = channels;
    fmt.sample_rate = sample_rate;
    switch (dsp_format) {
        case FORMAT_U8:
            fmt.audio_format = WAVE_LPCM;
            fmt.bits_per_sample = 8;
            break;
        case FORMAT_S16:
            fmt.audio_format = WAVE_LPCM;
            fmt.bits_per_sample = 16;
            break;
        case FORMAT_S24:
            fmt.audio_format = WAVE_LPCM;
            fmt.bits_per_sample = 24;
            break;
        case FORMAT_F32:
            fmt.audio_format = WAVE_IEEE_FLOAT;
            fmt.bits_per_sample = 32;
            break;
        case FORMAT_UNKNOWN:
            // Undefined
            throw std::invalid_argument("WAVE: creating header with unknown DSP audio format: " +
                                        std::to_string(dsp_format));
    }
    fmt.block_align = channels * (fmt.bits_per_sample / 8);
    fmt.byte_rate = sample_rate * fmt.block_align;

    // Sizes
    fmt.size = 16;
    data.size = fmt.block_align * samples_per_channel;
    riff.size = CHUNK_ID_SIZE + (CHUNK_HEADER_SIZE + fmt.size) + (CHUNK_HEADER_SIZE + data.size);
}

WaveHeader::WaveHeader(std::span<std::byte>& data_span) :
                       riff{}, fmt{}, data{} {
    if (data_span.size() < sizeof(RiffHeaderChunk)) {
        throw std::out_of_range("WAVE: data_span size smaller than RIFF header chunk");
    }

    // 1. RIFF header chunk
    std::copy(data_span.begin(), data_span.begin() + sizeof(riff),
              reinterpret_cast<std::byte*>(&riff));
    if (riff.size < CHUNK_HEADER_SIZE || riff.size + CHUNK_HEADER_SIZE > data_span.size()) {
        throw std::out_of_range("WAVE: invalid RIFF chunk size: " + std::to_string(riff.size));
    }
    riff.validate();
    data_span = data_span.subspan(sizeof(riff));

    // 2. Format and data_span sub-chunks
    while (data_span.size() >= 8) {
        auto chunk_id = data_span.data();
        auto chunk_size = *reinterpret_cast<uint32_t*>(data_span.data() + CHUNK_ID_SIZE);
        if (data_span.size() < chunk_size + CHUNK_HEADER_SIZE) {
            throw std::out_of_range("WAVE: data_span size < total chunk size: " +
                                    std::to_string(chunk_size + CHUNK_HEADER_SIZE));
        }

        if (!memcmp(chunk_id, CHUNK_ID_FMT, CHUNK_ID_SIZE) &&
                chunk_size >= sizeof(fmt) - CHUNK_HEADER_SIZE) {
            std::copy(data_span.begin(), data_span.begin() + sizeof(fmt),
                      reinterpret_cast<std::byte*>(&fmt));
            fmt.validate();
        } else if (!memcmp(chunk_id, CHUNK_ID_DATA, CHUNK_ID_SIZE) &&
                chunk_size >= sizeof(data) - CHUNK_HEADER_SIZE) {
            std::copy(data_span.begin(), data_span.begin() + sizeof(data),
                      reinterpret_cast<std::byte*>(&data));

            // Remove the chunk header and return if this is the data chunk
            data_span = data_span.subspan(CHUNK_HEADER_SIZE);
            break;
        }

        data_span = data_span.subspan(chunk_size + CHUNK_HEADER_SIZE);
    }

    if (!fmt.id[0]) {
        throw std::invalid_argument("WAVE: format chunk not found");
    }
    if (!data.id[0]) {
        throw std::invalid_argument("WAVE: data chunk not found");
    }
}

AudioFormat WaveHeader::getAudioFormat() const {
    if (fmt.audio_format == WAVE_LPCM) {
        switch (fmt.bits_per_sample) {
            case  8: return FORMAT_U8;
            case 16: return FORMAT_S16;
            case 24: return FORMAT_S24;
            default: return FORMAT_UNKNOWN;
        }
    } else if (fmt.audio_format == WAVE_IEEE_FLOAT) {
        if (fmt.bits_per_sample == 32) {
            return FORMAT_F32;
        } else {
            throw std::invalid_argument("WAVE: unsupported bits-per-sample for float type: " +
                                        std::to_string(fmt.bits_per_sample));
        }
    } else {
        throw std::invalid_argument("WAVE: unknown audio format " + std::to_string(fmt.audio_format));
    }
}

std::vector<std::vector<float>> load_wave_data_float(std::span<std::byte> data) {
    // Copy because WaveHeader mutates it
    std::span<std::byte> data_span = data;
    WaveHeader wave(data_span);

    auto num_samples = wave.data.size / (wave.fmt.bits_per_sample / 8);
    auto format = wave.getAudioFormat();

    std::vector<std::vector<float>> channel_bufs(wave.fmt.num_channels);
    if (format == FORMAT_F32) {
        auto float_data = reinterpret_cast<float*>(data_span.data());
        deinterleave_pcm(float_data, num_samples, channel_bufs);
    } else if (format == FORMAT_S16) {
        auto int_data = reinterpret_cast<short*>(data_span.data());
        std::vector<float> float_data(num_samples);
        for (auto i = 0; i < num_samples; i++) {
            float_data[i] = pcm_s16_to_float32(int_data[i]);
        }

        deinterleave_pcm(float_data.data(), num_samples, channel_bufs);
    } else {
        throw std::invalid_argument("WAVE: unsupported audio format " + std::to_string(format));
    }

    return channel_bufs;
}

std::vector<std::vector<float>> load_wave_file_float(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::vector<char> buf((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    file.close();

    auto data_span = *reinterpret_cast<std::vector<std::byte>*>(&buf);
    return load_wave_data_float(data_span);
}

void RiffHeaderChunk::validate() const {
    if (memcmp(id, CHUNK_ID_RIFF, CHUNK_ID_SIZE) != 0) {
        throw std::runtime_error("WAVE: invalid RIFF chunk ID");
    }

    if (memcmp(format, RIFF_FORMAT_WAVE, CHUNK_ID_SIZE) != 0) {
        throw std::runtime_error("WAVE: invalid RIFF format ID");
    }
}

void WaveFmtChunk::validate() const {
    // Sizes
    if (size < 16) {
        throw std::runtime_error("WAVE: invalid format chunk size: " + std::to_string(size));
    }
    if (byte_rate != sample_rate * block_align) {
        throw std::runtime_error("WAVE: byte rate (" + std::to_string(byte_rate) +
                                 ") != sample rate (" + std::to_string(sample_rate) +
                                 ") * block align (" + std::to_string(block_align));
    }
    if (block_align != num_channels * (bits_per_sample / 8)) {
        throw std::runtime_error("WAVE: block align (" + std::to_string(block_align) +
                                 ") != num channels (" + std::to_string(num_channels) +
                                 ") * bytes-per-sample (" + std::to_string(bits_per_sample / 8));
    }

    // Format: LPCM or float
    if (audio_format != WAVE_LPCM && audio_format != WAVE_IEEE_FLOAT) {
        throw std::runtime_error("WAVE: unknown audio format: " + std::to_string(audio_format));
    }

    // Supported sample size
    if (bits_per_sample < 8 || bits_per_sample > 32 || bits_per_sample % 8 != 0) {
        throw std::runtime_error("WAVE: unknown or invalid bits-per-sample: " + std::to_string(bits_per_sample));
    }
}

}
