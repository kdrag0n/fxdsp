#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

#include "../pcm.h"
#include "../wave.h"
#include "fr_sweep.h"

using namespace fxdsp;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [out.wav]\n";
        return 1;
    }

    std::vector<float> samples;
    samples.resize(SAMPLE_RATE * 3, 0.0f);
    for (int freq = MIN_FREQ; freq < MAX_FREQ; freq += FREQ_STEP) {
        float f = static_cast<float>(freq) / static_cast<float>(SAMPLE_RATE);
        for (int si = 0; si < samples.size(); si++) {
            samples[si] += AMPLITUDE * sin(2.0f * M_PI * f * si + PHASE);
        }
    }

    WaveHeader header(FORMAT_F32, 1, SAMPLE_RATE, samples.size());

    std::string out_path(argv[1]);
    std::ofstream out_file(out_path, std::ios::binary);
    out_file.write(reinterpret_cast<char*>(&header), sizeof(header));
    out_file.write(reinterpret_cast<char*>(samples.data()), samples.size() * sizeof(samples[0]));
    out_file.close();

    return 0;
}
