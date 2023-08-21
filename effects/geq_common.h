#pragma once

#include <vector>

#include "../dsp.h"

namespace fxdsp {

struct GraphicEqBand {
    float center_freq;
    float q;
    float gain_db;

    GraphicEqBand(float center_freq, float q, float gain_db);
};

class GraphicEqBase : public Effect {
private:
    virtual void build_filters() = 0;

protected:
    // Band state for (re)building filters
    std::vector<GraphicEqBand> bands;

public:
    GraphicEqBase(const DSP& dsp, int num_bands, float start_freq = 20.0f, float end_freq = 20000.0f);

    void init_bands(int num_bands, float start_freq, float end_freq); // resets state
    void set_all_bands(const std::vector<float>& gains);
    void set_band_gain(int band_idx, float gain_db);
    GraphicEqBand& get_band(int band_idx);
};

}
