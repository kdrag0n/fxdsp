#include "dsp.h"

#include <utility>

namespace fxdsp {

Effect::Effect(const DSP &dsp) :
        AudioSink(dsp.audio_format, dsp.channels),
        sink(nullptr) {
}

void Effect::set_next_sink(AudioSink &next_sink) {
    sink = &next_sink;
}

void Effect::reset() {
}

void Effect::finalize() {
}

DSP::DSP(AudioFormat audio_format, int sample_rate, int channels, AudioSink* sink) :
        AudioSink(audio_format, channels),
        sink(sink),
        sample_rate(sample_rate),
        channels(channels) {
    audio_buf.resize(channels);
}

void DSP::write_audio(std::vector<std::vector<float>>& buf) {
    if (!effect_chain.empty()) {
        effect_chain[0]->write_audio(buf);
    } else {
        sink->write_audio(buf);
    }
}

void DSP::add_effect(Effect* effect) {
    effect_chain.push_back(effect);
    update_sinks();
}

void DSP::remove_effect(Effect* effect) {
    effect_chain.erase(std::remove(effect_chain.begin(), effect_chain.end(), effect),
                       effect_chain.end());
    update_sinks();
}

const std::vector<Effect*>& DSP::get_effects() {
    return effect_chain;
}

void DSP::clear_effects() {
    effect_chain.clear();
}

void DSP::set_sink(AudioSink* new_sink) {
    sink = new_sink;
    update_sinks();
}

void DSP::update_sinks() {
    for (auto i = 0; i < effect_chain.size(); i++) {
        auto& effect = effect_chain[i];

        if (effect->enabled) {
            auto& next_sink = (i == effect_chain.size() - 1) ? *sink : *effect_chain[i+1];
            effect->set_next_sink(next_sink);
        }
    }
}

}
