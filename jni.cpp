#include <jni.h>
#include <string>

#include "dsp.h"
#include "log.h"
#include "effects/gain.h"
#include "effects/noise.h"
#include "effects/silence.h"
#include "types.h"
#include "effects/convolver.h"
#include "effects/graphic_eq_fir.h"
#include "effects/graphic_eq_iir.h"
#include "sinks/oboe.h"
#include "util/graph.h"
#include "wave.h"

namespace fxdsp {

extern "C" {

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nDspCreate(JNIEnv *env, jclass clazz, jint sample_rate, jint channels, jlong sink_ptr) {
    // Allocate like this as lifecycle is tied to Java object
    auto dsp = new DSP(FORMAT_S16, sample_rate, channels, reinterpret_cast<AudioSink*>(sink_ptr));
    return reinterpret_cast<long>(dsp);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nDspWriteAudio(
        JNIEnv* env,
        jclass clazz,
        jlong dsp_ptr,
        jshortArray java_buf,
        jlong float_2d_buf,
        jint sample_count) {
    auto raw_buf = env->GetShortArrayElements(java_buf, nullptr);
    auto& float_buf = *reinterpret_cast<std::vector<std::vector<float>>*>(float_2d_buf);

    // We've already allocated max size. To avoid writing garbage samples, resize to the part we'll
    // actually use.
    for (auto& ch : float_buf) {
        ch.resize(sample_count / float_buf.size());
    }

    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    deinterleave_pcm_s16(raw_buf, sample_count, float_buf);
    dsp->write_audio(float_buf);

    env->ReleaseShortArrayElements(java_buf, raw_buf, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nDspAddEffect(JNIEnv *env, jclass clazz,
                                                      jlong dsp_ptr, jlong effect_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = reinterpret_cast<Effect*>(effect_ptr);

    dsp->add_effect(effect);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nDspRemoveEffect(JNIEnv *env, jclass clazz,
                                                         jlong dsp_ptr, jlong effect_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = reinterpret_cast<Effect*>(effect_ptr);

    dsp->remove_effect(effect);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nDspSetSink(JNIEnv *env, jclass clazz, jlong dsp_ptr,
                                                    jlong sink_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto sink = reinterpret_cast<AudioSink*>(sink_ptr);
    dsp->set_sink(sink);
}

JNIEXPORT jboolean JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nEffectGetEnabled(JNIEnv *env, jclass clazz,
                                                          jlong effect_ptr) {
    auto effect = reinterpret_cast<Effect*>(effect_ptr);
    return effect->enabled;
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nEffectSetEnabled(JNIEnv *env, jclass clazz, jlong effect_ptr,
                                                          jboolean enabled) {
    auto effect = reinterpret_cast<Effect*>(effect_ptr);
    effect->enabled = enabled;
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nConvolverEffectCreate(JNIEnv *env,
                                                               jclass clazz,
                                                               jlong dsp_ptr,
                                                               jint block_size) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = new ConvolverEffect(*dsp, block_size);
    return reinterpret_cast<long>(effect);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGainEffectCreate(JNIEnv *env,
                                                          jclass clazz,
                                                          jlong dsp_ptr,
                                                          jfloat gain_db) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = new GainEffect(*dsp, gain_db);
    return reinterpret_cast<long>(effect);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqFirEffectCreate(JNIEnv *env,
                                                            jclass clazz,
                                                            jlong dsp_ptr,
                                                            jint num_bands,
                                                            jint block_size,
                                                            float start_freq,
                                                            float end_freq) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = new FirGraphicEqEffect(*dsp, num_bands, block_size, start_freq, end_freq);
    return reinterpret_cast<long>(effect);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nNoiseEffectCreate(JNIEnv *env,
                                                           jclass clazz,
                                                           jlong dsp_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = new NoiseEffect(*dsp);
    return reinterpret_cast<long>(effect);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nSilenceEffectCreate(JNIEnv *env,
                                                             jclass clazz,
                                                             jlong dsp_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = new SilenceEffect(*dsp);
    return reinterpret_cast<long>(effect);
}

JNIEXPORT jlongArray JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nDspGetEffects(JNIEnv *env, jclass clazz, jlong dsp_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto& effects = dsp->get_effects();

    auto array = env->NewLongArray(static_cast<jsize>(effects.size()));
    std::vector<jlong> effects64(effects.size());
    for (int i = 0; i < effects.size(); i++) {
        effects64[i] = reinterpret_cast<jlong>(effects[i]);
    }
    env->SetLongArrayRegion(array, 0, static_cast<jsize>(effects.size()), effects64.data());
    return array;
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nDspClearEffects(JNIEnv *env, jclass clazz, jlong dsp_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    dsp->clear_effects();
}

JNIEXPORT jfloat JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqBandGetCenterFreq(JNIEnv *env, jclass clazz,
                                                              jlong band_ptr) {
    auto band = reinterpret_cast<GraphicEqBand*>(band_ptr);
    return band->center_freq;
}

JNIEXPORT jfloat JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqBandGetQ(JNIEnv *env, jclass clazz, jlong band_ptr) {
    auto band = reinterpret_cast<GraphicEqBand*>(band_ptr);
    return band->q;
}

JNIEXPORT jfloat JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqBandGetGainDb(JNIEnv *env, jclass clazz,
                                                          jlong band_ptr) {
    auto band = reinterpret_cast<GraphicEqBand*>(band_ptr);
    return band->gain_db;
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqFirEffectFinalize(JNIEnv *env, jclass clazz,
                                                              jlong effect_ptr) {
    auto effect = reinterpret_cast<FirGraphicEqEffect*>(effect_ptr);
    effect->finalize();
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqFirEffectInitBands(JNIEnv *env, jclass clazz,
                                                               jlong effect_ptr, jint num_bands,
                                                               jfloat start_freq, jfloat end_freq) {
    auto effect = reinterpret_cast<FirGraphicEqEffect*>(effect_ptr);
    effect->init_bands(num_bands, start_freq, end_freq);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqFirEffectSetAllBands(JNIEnv *env, jclass clazz,
                                                                 jlong effect_ptr,
                                                                 jfloatArray gains) {
    auto effect = reinterpret_cast<FirGraphicEqEffect*>(effect_ptr);
    std::vector<float> vec(env->GetArrayLength(gains));
    env->GetFloatArrayRegion(gains, 0, env->GetArrayLength(gains), vec.data());
    effect->set_all_bands(vec);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqFirEffectSetBandGain(JNIEnv *env, jclass clazz,
                                                                 jlong effect_ptr, jint band_idx,
                                                                 jfloat gain_db) {
    auto effect = reinterpret_cast<FirGraphicEqEffect*>(effect_ptr);
    effect->set_band_gain(band_idx, gain_db);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqFirEffectGetBand(JNIEnv *env, jclass clazz,
                                                             jlong effect_ptr, jint band_idx) {
    auto effect = reinterpret_cast<FirGraphicEqEffect*>(effect_ptr);
    auto& band = effect->get_band(band_idx);
    return reinterpret_cast<jlong>(&band);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqIirEffectCreate(JNIEnv *env, jclass clazz,
                                                            jlong dsp_ptr, jint num_bands,
                                                            jfloat start_freq, jfloat end_freq) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = new IirGraphicEqEffect(*dsp, num_bands, start_freq, end_freq);
    return reinterpret_cast<jlong>(effect);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqIirEffectInitBands(JNIEnv *env, jclass clazz,
                                                               jlong effect_ptr, jint num_bands,
                                                               jfloat start_freq, jfloat end_freq) {
    auto effect = reinterpret_cast<IirGraphicEqEffect*>(effect_ptr);
    effect->init_bands(num_bands, start_freq, end_freq);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqIirEffectSetAllBands(JNIEnv *env, jclass clazz,
                                                                 jlong effect_ptr,
                                                                 jfloatArray gains) {
    auto effect = reinterpret_cast<IirGraphicEqEffect*>(effect_ptr);
    std::vector<float> vec(env->GetArrayLength(gains));
    env->GetFloatArrayRegion(gains, 0, env->GetArrayLength(gains), vec.data());
    effect->set_all_bands(vec);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqIirEffectSetBandGain(JNIEnv *env, jclass clazz,
                                                                 jlong effect_ptr, jint band_idx,
                                                                 jfloat gain_db) {
    auto effect = reinterpret_cast<IirGraphicEqEffect*>(effect_ptr);
    effect->set_band_gain(band_idx, gain_db);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGeqIirEffectGetBand(JNIEnv *env, jclass clazz,
                                                             jlong effect_ptr, jint band_idx) {
    auto effect = reinterpret_cast<IirGraphicEqEffect*>(effect_ptr);
    auto& band = effect->get_band(band_idx);
    return reinterpret_cast<jlong>(&band);
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nPeqEffectCreate(JNIEnv *env, jclass clazz, jlong dsp_ptr) {
    auto dsp = reinterpret_cast<DSP*>(dsp_ptr);
    auto effect = new ParametricEqEffect(*dsp);
    return reinterpret_cast<jlong>(effect);
}

JNIEXPORT jint JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nPeqEffectAddFilter(JNIEnv *env, jclass clazz,
                                                            jlong effect_ptr, jint type,
                                                            jfloat center_freq, jfloat q,
                                                            jfloat gain_db) {
    auto effect = reinterpret_cast<ParametricEqEffect*>(effect_ptr);
    return static_cast<jint>(effect->add_filter(static_cast<BiquadFilterType>(type), center_freq, q, gain_db));
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nPeqEffectUpdateFilter(JNIEnv *env, jclass clazz,
                                                               jlong effect_ptr, jint idx,
                                                               jint type, jfloat center_freq,
                                                               jfloat q, jfloat gain_db) {
    auto effect = reinterpret_cast<ParametricEqEffect*>(effect_ptr);
    effect->update_filter(idx, static_cast<BiquadFilterType>(type), center_freq, q, gain_db);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nPeqEffectRemoveFilter(JNIEnv *env, jclass clazz,
                                                               jlong effect_ptr, jint idx) {
    auto effect = reinterpret_cast<ParametricEqEffect*>(effect_ptr);
    effect->remove_filter(idx);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nPeqEffectRemoveAllFilters(JNIEnv *env, jclass clazz,
                                                                   jlong effect_ptr) {
    auto effect = reinterpret_cast<ParametricEqEffect*>(effect_ptr);
    effect->remove_all_filters();
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nOboeSinkCreate(JNIEnv *env, jclass clazz, jint channels, jint session_id) {
    auto sink = new OboeSink(channels, session_id);
    return reinterpret_cast<jlong>(sink);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nOboeSinkOpen(JNIEnv *env, jclass clazz, jlong sink_ptr) {
    auto sink = reinterpret_cast<OboeSink*>(sink_ptr);
    sink->open();
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nOboeSinkClose(JNIEnv *env, jclass clazz, jlong sink_ptr) {
    auto sink = reinterpret_cast<OboeSink*>(sink_ptr);
    sink->close();
}

JNIEXPORT jint JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nOboeSinkGetBufferSize(JNIEnv *env, jclass clazz,
                                                               jlong sink_ptr) {
    auto sink = reinterpret_cast<OboeSink*>(sink_ptr);
    return sink->buffer_size;
}

JNIEXPORT jlong JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nResizeFloat2d(JNIEnv *env, jclass clazz, jlong old_ptr,
                                                       jint channels, jint samples) {
    auto buf = reinterpret_cast<std::vector<std::vector<float>>*>(old_ptr);
    if (buf == nullptr) {
        buf = new std::vector<std::vector<float>>(channels);
    } else {
        buf->resize(channels);
    }

    for (auto& ch : *buf) {
        ch.resize(samples);
    }

    return reinterpret_cast<jlong>(buf);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nFree(JNIEnv *env,
                                              jclass clazz,
                                              jlong ptr) {
    auto obj = reinterpret_cast<void*>(ptr);
    free(obj);
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGraphInterpolate(JNIEnv *env, jclass clazz,
                                                          jfloatArray in_x, jfloatArray in_y,
                                                          jfloatArray out_x, jfloatArray out_y) {
    std::vector<float> in_x_vec(env->GetArrayLength(in_x));
    std::vector<float> in_y_vec(env->GetArrayLength(in_y));
    env->GetFloatArrayRegion(in_x, 0, static_cast<jsize>(in_x_vec.size()), in_x_vec.data());
    env->GetFloatArrayRegion(in_y, 0, static_cast<jsize>(in_y_vec.size()), in_y_vec.data());

    std::vector<float> out_x_vec(env->GetArrayLength(out_x));
    std::vector<float> out_y_vec(env->GetArrayLength(out_y));
    graph::interpolate(in_x_vec, in_y_vec, out_x_vec, out_y_vec);

    env->SetFloatArrayRegion(out_x, 0, static_cast<jsize>(out_x_vec.size()), out_x_vec.data());
    env->SetFloatArrayRegion(out_y, 0, static_cast<jsize>(out_y_vec.size()), out_y_vec.data());
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGraphBiquadDb(JNIEnv *env, jclass clazz, jfloatArray out_x,
                                                       jfloatArray out_y, jfloat max_freq,
                                                       jint type, jfloat sample_rate,
                                                       jfloat center_freq, jfloat q,
                                                       jfloat gain_db) {
    std::vector<float> out_x_vec(env->GetArrayLength(out_x));
    std::vector<float> out_y_vec(env->GetArrayLength(out_y));
    BiquadFilter filter(static_cast<BiquadFilterType>(type), sample_rate, center_freq, q, gain_db);
    filter.gen_graph(out_x_vec, out_y_vec, max_freq);

    env->SetFloatArrayRegion(out_x, 0, static_cast<jsize>(out_x_vec.size()), out_x_vec.data());
    env->SetFloatArrayRegion(out_y, 0, static_cast<jsize>(out_y_vec.size()), out_y_vec.data());
}

JNIEXPORT void JNICALL
Java_dev_kdrag0n_audiofx_core_NativeLib_nGraphIrWav(JNIEnv *env, jclass clazz, jstring path_java,
                                                    jfloatArray ir_out_x, jfloatArray ir_out_y,
                                                    jfloatArray fr_out_x, jfloatArray fr_out_y,
                                                    jfloatArray pr_out_x, jfloatArray pr_out_y) {
    std::vector<float> ir_out_x_vec(env->GetArrayLength(ir_out_x));
    std::vector<float> ir_out_y_vec(env->GetArrayLength(ir_out_y));
    std::vector<float> fr_out_x_vec(env->GetArrayLength(fr_out_x));
    std::vector<float> fr_out_y_vec(env->GetArrayLength(fr_out_y));
    std::vector<float> pr_out_x_vec(env->GetArrayLength(pr_out_x));
    std::vector<float> pr_out_y_vec(env->GetArrayLength(pr_out_y));

    auto path_data = env->GetStringUTFChars(path_java, nullptr);
    std::string path(path_data, env->GetStringLength(path_java));

    auto ir_channels = load_wave_file_float(path);
    auto ir = ir_channels[0]; // TODO: bound check

    auto max_freq = 22000.0f; // TODO
    graph::impulse_response_curves(ir, ir_out_x_vec, ir_out_y_vec, fr_out_x_vec,
                                   fr_out_y_vec, pr_out_x_vec, pr_out_y_vec, max_freq);

    env->SetFloatArrayRegion(ir_out_x, 0, static_cast<jsize>(ir_out_x_vec.size()), ir_out_x_vec.data());
    env->SetFloatArrayRegion(ir_out_y, 0, static_cast<jsize>(ir_out_y_vec.size()), ir_out_y_vec.data());
    env->SetFloatArrayRegion(fr_out_x, 0, static_cast<jsize>(fr_out_x_vec.size()), fr_out_x_vec.data());
    env->SetFloatArrayRegion(fr_out_y, 0, static_cast<jsize>(fr_out_y_vec.size()), fr_out_y_vec.data());
    env->SetFloatArrayRegion(pr_out_x, 0, static_cast<jsize>(pr_out_x_vec.size()), pr_out_x_vec.data());
    env->SetFloatArrayRegion(pr_out_y, 0, static_cast<jsize>(pr_out_y_vec.size()), pr_out_y_vec.data());
    env->ReleaseStringUTFChars(path_java, path_data);
}

}

}
