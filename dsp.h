#pragma once

#include <vector>
#include <memory>

#include "pcm.h"
#include "types.h"
#include "sink.h"

namespace fxdsp {

class DSP;

// Lifecycle managed by Java
class Effect : public AudioSink {
protected:
    // Initialized at attach time
    AudioSink* sink;

public:
    Effect(const DSP& dsp);
    virtual ~Effect() {}

    bool enabled = true;

    // Virtual so effects can delegate to encapsulated effects
    virtual void set_next_sink(AudioSink& next_sink);

    virtual void reset();
    virtual void finalize();
};

class DSP : public AudioSink {
private:
    std::vector<Effect*> effect_chain;
    std::vector<std::vector<float>> audio_buf;
    AudioSink* sink;

    void update_sinks();

public:
    DSP(AudioFormat audio_format, int sample_rate, int channels, AudioSink* sink);

    int sample_rate;
    int channels;

    // F32 [channel samples]
    void write_audio(std::vector<std::vector<float>>& buf) override;

    void add_effect(Effect* effect);
    void remove_effect(Effect* effect);
    const std::vector<Effect*>& get_effects();
    void clear_effects();

    void set_sink(AudioSink* new_sink);
};

}
