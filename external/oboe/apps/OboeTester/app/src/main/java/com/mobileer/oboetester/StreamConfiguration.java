/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.mobileer.oboetester;

import android.content.res.Resources;

import java.util.HashMap;

/**
 * Container for the properties of a Stream.
 *
 * This can be used to build a stream, or as a base class for a Stream,
 * or as a way to report the properties of a Stream.
 */

public class StreamConfiguration {
    public static final int UNSPECIFIED = 0;

    // These must match order in Spinner and in native code and in AAudio.h
    public static final int NATIVE_API_UNSPECIFIED = 0;
    public static final int NATIVE_API_OPENSLES = 1;
    public static final int NATIVE_API_AAUDIO = 2;

    public static final int SHARING_MODE_EXCLUSIVE = 0; // must match AAUDIO
    public static final int SHARING_MODE_SHARED = 1; // must match AAUDIO

    public static final int AUDIO_FORMAT_PCM_16 = 1; // must match AAUDIO
    public static final int AUDIO_FORMAT_PCM_FLOAT = 2; // must match AAUDIO
    public static final int AUDIO_FORMAT_PCM_24 = 3; // must match AAUDIO
    public static final int AUDIO_FORMAT_PCM_32 = 4; // must match AAUDIO

    public static final int DIRECTION_OUTPUT = 0; // must match AAUDIO
    public static final int DIRECTION_INPUT = 1; // must match AAUDIO

    public static final int SESSION_ID_NONE = -1; // must match AAUDIO
    public static final int SESSION_ID_ALLOCATE = 0; // must match AAUDIO

    public static final int PERFORMANCE_MODE_NONE = 10; // must match AAUDIO
    public static final int PERFORMANCE_MODE_POWER_SAVING = 11; // must match AAUDIO
    public static final int PERFORMANCE_MODE_LOW_LATENCY = 12; // must match AAUDIO

    public static final int RATE_CONVERSION_QUALITY_NONE = 0; // must match Oboe
    public static final int RATE_CONVERSION_QUALITY_FASTEST = 1; // must match Oboe
    public static final int RATE_CONVERSION_QUALITY_LOW = 2; // must match Oboe
    public static final int RATE_CONVERSION_QUALITY_MEDIUM = 3; // must match Oboe
    public static final int RATE_CONVERSION_QUALITY_HIGH = 4; // must match Oboe
    public static final int RATE_CONVERSION_QUALITY_BEST = 5; // must match Oboe

    public static final int STREAM_STATE_STARTING = 3; // must match Oboe
    public static final int STREAM_STATE_STARTED = 4; // must match Oboe

    public static final int INPUT_PRESET_GENERIC = 1; // must match Oboe
    public static final int INPUT_PRESET_CAMCORDER = 5; // must match Oboe
    public static final int INPUT_PRESET_VOICE_RECOGNITION = 6; // must match Oboe
    public static final int INPUT_PRESET_VOICE_COMMUNICATION = 7; // must match Oboe
    public static final int INPUT_PRESET_UNPROCESSED = 9; // must match Oboe
    public static final int INPUT_PRESET_VOICE_PERFORMANCE = 10; // must match Oboe

    public static final int ERROR_DISCONNECTED = -899; // must match Oboe

    public static final int USAGE_MEDIA = 1;
    public static final int USAGE_VOICE_COMMUNICATION = 2;
    public static final int USAGE_VOICE_COMMUNICATION_SIGNALLING = 3;
    public static final int USAGE_ALARM = 4;
    public static final int USAGE_NOTIFICATION = 5;
    public static final int USAGE_NOTIFICATION_RINGTONE = 6;
    public static final int USAGE_NOTIFICATION_EVENT = 10;
    public static final int USAGE_ASSISTANCE_ACCESSIBILITY = 11;
    public static final int USAGE_ASSISTANCE_NAVIGATION_GUIDANCE = 12;
    public static final int USAGE_ASSISTANCE_SONIFICATION = 13;
    public static final int USAGE_GAME = 14;
    public static final int USAGE_ASSISTANT = 16;

    public static final int[] usages = {
             USAGE_MEDIA,
            USAGE_VOICE_COMMUNICATION,
            USAGE_VOICE_COMMUNICATION_SIGNALLING,
            USAGE_ALARM,
            USAGE_NOTIFICATION,
            USAGE_NOTIFICATION_RINGTONE,
            USAGE_NOTIFICATION_EVENT,
            USAGE_ASSISTANCE_ACCESSIBILITY,
            USAGE_ASSISTANCE_NAVIGATION_GUIDANCE,
            USAGE_ASSISTANCE_SONIFICATION,
            USAGE_GAME,
            USAGE_ASSISTANT};

    private int mNativeApi;
    private int mBufferCapacityInFrames;
    private int mChannelCount;
    private int mDeviceId;
    private int mSessionId;
    private int mDirection; // does not get reset
    private int mFormat;
    private int mSampleRate;
    private int mSharingMode;
    private int mPerformanceMode;
    private boolean mFormatConversionAllowed;
    private boolean mChannelConversionAllowed;
    private int mRateConversionQuality;
    private int mInputPreset;
    private int mUsage;
    private static HashMap<String,Integer> mUsageStringToIntegerMap;

    private int mFramesPerBurst = 0;

    private boolean mMMap = false;

    public StreamConfiguration() {
        reset();
    }

    static {
        // Build map for Usage string-to-int conversion.
        mUsageStringToIntegerMap = new HashMap<String,Integer>();
        for (int usage : usages) {
            mUsageStringToIntegerMap.put(convertUsageToText(usage), usage);
        }
    }

    public void reset() {
        mNativeApi = NATIVE_API_UNSPECIFIED;
        mBufferCapacityInFrames = UNSPECIFIED;
        mChannelCount = UNSPECIFIED;
        mDeviceId = UNSPECIFIED;
        mSessionId = -1;
        mFormat = AUDIO_FORMAT_PCM_FLOAT;
        mSampleRate = UNSPECIFIED;
        mSharingMode = SHARING_MODE_EXCLUSIVE;
        mPerformanceMode = PERFORMANCE_MODE_LOW_LATENCY;
        mInputPreset = INPUT_PRESET_VOICE_RECOGNITION;
        mUsage = 0; // FIXME add USAGE_*
        mFormatConversionAllowed = false;
        mChannelConversionAllowed = false;
        mRateConversionQuality = RATE_CONVERSION_QUALITY_NONE;
        mMMap = NativeEngine.isMMapSupported();
    }

    public int getFramesPerBurst() {
        return mFramesPerBurst;
    }

    public void setFramesPerBurst(int framesPerBurst) {
        this.mFramesPerBurst = framesPerBurst;
    }

    public int getBufferCapacityInFrames() {
        return mBufferCapacityInFrames;
    }

    public void setBufferCapacityInFrames(int bufferCapacityInFrames) {
        this.mBufferCapacityInFrames = bufferCapacityInFrames;
    }

    public int getFormat() {
        return mFormat;
    }

    public void setFormat(int format) {
        this.mFormat = format;
    }

    public int getDirection() {
        return mDirection;
    }

    public void setDirection(int direction) {
        this.mDirection = direction;
    }

    public int getPerformanceMode() {
        return mPerformanceMode;
    }

    public void setPerformanceMode(int performanceMode) {
        this.mPerformanceMode = performanceMode;
    }

    static String convertPerformanceModeToText(int performanceMode) {
        switch(performanceMode) {
            case PERFORMANCE_MODE_NONE:
                return "NO";
            case PERFORMANCE_MODE_POWER_SAVING:
                return "PS";
            case PERFORMANCE_MODE_LOW_LATENCY:
                return "LL";
            default:
                return "??";
        }
    }

    public int getInputPreset() { return mInputPreset; }
    public void setInputPreset(int inputPreset) {
        this.mInputPreset = inputPreset;
    }

    public int getUsage() { return mUsage; }
    public void setUsage(int Usage) {
        this.mUsage = Usage;
    }

    static String convertUsageToText(int usage) {
        switch(usage) {
            case USAGE_MEDIA:
                return "Media";
            case USAGE_VOICE_COMMUNICATION:
                return "VoiceComm";
            case USAGE_VOICE_COMMUNICATION_SIGNALLING:
                return "VoiceCommSig";
            case USAGE_ALARM:
                return "Alarm";
            case USAGE_NOTIFICATION:
                return "Notification";
            case USAGE_NOTIFICATION_RINGTONE:
                return "Ringtone";
            case USAGE_NOTIFICATION_EVENT:
                return "Event";
            case USAGE_ASSISTANCE_ACCESSIBILITY:
                return "Accessability";
            case USAGE_ASSISTANCE_NAVIGATION_GUIDANCE:
                return "Navigation";
            case USAGE_ASSISTANCE_SONIFICATION:
                return "Sonification";
            case USAGE_GAME:
                return "Game";
            case USAGE_ASSISTANT:
                return "Assistant";
            default:
                return "?=" + usage;
        }
    }

    public static int convertTextToUsage(String text) {
        return mUsageStringToIntegerMap.get(text);
    }

    public int getSharingMode() {
        return mSharingMode;
    }

    public void setSharingMode(int sharingMode) {
        this.mSharingMode = sharingMode;
    }

    static String convertSharingModeToText(int sharingMode) {
        switch(sharingMode) {
            case SHARING_MODE_SHARED:
                return "SH";
            case SHARING_MODE_EXCLUSIVE:
                return "EX";
            default:
                return "??";
        }
    }

    public static String convertFormatToText(int format) {
        switch(format) {
            case UNSPECIFIED:
                return "Unspecified";
            case AUDIO_FORMAT_PCM_16:
                return "I16";
            case AUDIO_FORMAT_PCM_24:
                return "I24";
            case AUDIO_FORMAT_PCM_32:
                return "I32";
            case AUDIO_FORMAT_PCM_FLOAT:
                return "Float";
            default:
                return "Invalid";
        }
    }

    public static String convertNativeApiToText(int api) {
        switch(api) {
            case NATIVE_API_UNSPECIFIED:
                return "Unspec";
            case NATIVE_API_AAUDIO:
                return "AAudio";
            case NATIVE_API_OPENSLES:
                return "OpenSL";
            default:
                return "Invalid";
        }
    }


    public String dump() {
        String prefix = (getDirection() == DIRECTION_INPUT) ? "in" : "out";
        StringBuffer message = new StringBuffer();
        message.append(String.format("%s.channels = %d\n", prefix, mChannelCount));
        message.append(String.format("%s.perf = %s\n", prefix,
                convertPerformanceModeToText(mPerformanceMode).toLowerCase()));
        if (getDirection() == DIRECTION_INPUT) {
            message.append(String.format("%s.preset = %s\n", prefix,
                    convertInputPresetToText(mInputPreset).toLowerCase()));
        } else {
            message.append(String.format("%s.preset = %s\n", prefix,
                    convertUsageToText(mUsage).toLowerCase()));
        }
        message.append(String.format("%s.sharing = %s\n", prefix,
                convertSharingModeToText(mSharingMode).toLowerCase()));
        message.append(String.format("%s.api = %s\n", prefix,
                convertNativeApiToText(getNativeApi()).toLowerCase()));
        message.append(String.format("%s.rate = %d\n", prefix, mSampleRate));
        message.append(String.format("%s.device = %d\n", prefix, mDeviceId));
        message.append(String.format("%s.mmap = %s\n", prefix, isMMap() ? "yes" : "no"));
        message.append(String.format("%s.rate.conversion.quality = %d\n", prefix, mRateConversionQuality));
        return message.toString();
    }

    // text must match menu values
    public static final String NAME_INPUT_PRESET_GENERIC = "Generic";
    public static final String NAME_INPUT_PRESET_CAMCORDER = "Camcorder";
    public static final String NAME_INPUT_PRESET_VOICE_RECOGNITION = "VoiceRec";
    public static final String NAME_INPUT_PRESET_VOICE_COMMUNICATION = "VoiceComm";
    public static final String NAME_INPUT_PRESET_UNPROCESSED = "Unprocessed";
    public static final String NAME_INPUT_PRESET_VOICE_PERFORMANCE = "Performance";

    public static String convertInputPresetToText(int inputPreset) {
        switch(inputPreset) {
            case INPUT_PRESET_GENERIC:
                return NAME_INPUT_PRESET_GENERIC;
            case INPUT_PRESET_CAMCORDER:
                return NAME_INPUT_PRESET_CAMCORDER;
            case INPUT_PRESET_VOICE_RECOGNITION:
                return NAME_INPUT_PRESET_VOICE_RECOGNITION;
            case INPUT_PRESET_VOICE_COMMUNICATION:
                return NAME_INPUT_PRESET_VOICE_COMMUNICATION;
            case INPUT_PRESET_UNPROCESSED:
                return NAME_INPUT_PRESET_UNPROCESSED;
            case INPUT_PRESET_VOICE_PERFORMANCE:
                return NAME_INPUT_PRESET_VOICE_PERFORMANCE;
            default:
                return "Invalid";
        }
    }

    private static boolean matchInputPreset(String text, int preset) {
        return convertInputPresetToText(preset).toLowerCase().equals(text);
    }

    /**
     * Case insensitive.
     * @param text
     * @return inputPreset, eg. INPUT_PRESET_CAMCORDER
     */
    public static int convertTextToInputPreset(String text) {
        text = text.toLowerCase();
        if (matchInputPreset(text, INPUT_PRESET_GENERIC)) {
            return INPUT_PRESET_GENERIC;
        } else if (matchInputPreset(text, INPUT_PRESET_CAMCORDER)) {
            return INPUT_PRESET_CAMCORDER;
        } else if (matchInputPreset(text, INPUT_PRESET_VOICE_RECOGNITION)) {
            return INPUT_PRESET_VOICE_RECOGNITION;
        } else if (matchInputPreset(text, INPUT_PRESET_VOICE_COMMUNICATION)) {
            return INPUT_PRESET_VOICE_COMMUNICATION;
        } else if (matchInputPreset(text, INPUT_PRESET_UNPROCESSED)) {
            return INPUT_PRESET_UNPROCESSED;
        } else if (matchInputPreset(text, INPUT_PRESET_VOICE_PERFORMANCE)) {
            return INPUT_PRESET_VOICE_PERFORMANCE;
        }
        return -1;
    }

    public int getChannelCount() {
        return mChannelCount;
    }

    public void setChannelCount(int channelCount) {
        this.mChannelCount = channelCount;
    }

    public int getSampleRate() {
        return mSampleRate;
    }

    public void setSampleRate(int sampleRate) {
        this.mSampleRate = sampleRate;
    }

    public int getDeviceId() {
        return mDeviceId;
    }

    public void setDeviceId(int deviceId) {
        this.mDeviceId = deviceId;
    }

    public int getSessionId() {
        return mSessionId;
    }

    public void setSessionId(int sessionId) {
        mSessionId = sessionId;
    }

    public boolean isMMap() {
        return mMMap;
    }

    public void setMMap(boolean b) { mMMap = b; }

    public int getNativeApi() {
        return mNativeApi;
    }

    public void setNativeApi(int nativeApi) {
        mNativeApi = nativeApi;
    }

    public void setChannelConversionAllowed(boolean b) { mChannelConversionAllowed = b; }

    public boolean getChannelConversionAllowed() {
        return mChannelConversionAllowed;
    }

    public void setFormatConversionAllowed(boolean b) {
        mFormatConversionAllowed = b;
    }

    public boolean getFormatConversionAllowed() {
        return mFormatConversionAllowed;
    }

    public void setRateConversionQuality(int quality) { mRateConversionQuality = quality; }

    public int getRateConversionQuality() {
        return mRateConversionQuality;
    }

}
