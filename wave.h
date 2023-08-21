#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <span>
#include <cstdint>
#include <cstddef>

#include "dsp.h"

namespace fxdsp {

static constexpr auto CHUNK_ID_SIZE = 4;
static constexpr auto CHUNK_HEADER_SIZE = CHUNK_ID_SIZE + sizeof(uint32_t);

static constexpr auto CHUNK_ID_RIFF = "RIFF";
static constexpr auto CHUNK_ID_FMT = "fmt ";
static constexpr auto CHUNK_ID_DATA = "data";

static constexpr auto RIFF_FORMAT_WAVE = "WAVE";

enum WaveAudioFormat : uint16_t  {
    WAVE_LPCM = 1,
    WAVE_IEEE_FLOAT = 3,
};

struct RiffHeaderChunk {
    char id[4]; // "RIFF"
    uint32_t size; // 4 + (8 + fmt_chunk.size) + (8 + data_chunk.size)
    char format[4]; // "WAVE"

    void validate() const;
} __attribute__((packed));

struct WaveFmtChunk {
    char id[4]; // "fmt "
    uint32_t size; // 16

    WaveAudioFormat audio_format; // LPCM = 1
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; // sample_rate * block_align
    uint16_t block_align; // num_channels * bytes_per_sample
    uint16_t bits_per_sample;

    void validate() const;
} __attribute__((packed));

struct WaveDataChunk {
    char id[4]; // "data"
    uint32_t size; // PCM data size

    std::byte data[0]; // PCM data
} __attribute__((packed));

struct WaveHeader {
    RiffHeaderChunk riff;
    WaveFmtChunk fmt;
    WaveDataChunk data;

    WaveHeader(AudioFormat dsp_format, uint16_t channels, uint32_t sample_rate, size_t samples_per_channel);
    // span will be mutated, remaining span = data payload
    WaveHeader(std::span<std::byte>& data_span);
    AudioFormat getAudioFormat() const;
} __attribute__((packed));

std::vector<std::vector<float>> load_wave_data_float(std::span<std::byte> data);
std::vector<std::vector<float>> load_wave_file_float(const std::string& in_path);

}
