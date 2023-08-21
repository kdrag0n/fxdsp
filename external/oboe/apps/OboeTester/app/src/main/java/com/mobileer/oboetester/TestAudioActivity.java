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

import android.app.Activity;
import android.content.Context;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Toast;

import java.io.IOException;
import java.util.ArrayList;

/**
 * Base class for other Activities.
 */
abstract class TestAudioActivity extends Activity {
    public static final String TAG = "OboeTester";

    protected static final int FADER_PROGRESS_MAX = 1000;

    public static final int AUDIO_STATE_OPEN = 0;
    public static final int AUDIO_STATE_STARTED = 1;
    public static final int AUDIO_STATE_PAUSED = 2;
    public static final int AUDIO_STATE_STOPPED = 3;
    public static final int AUDIO_STATE_CLOSING = 4;
    public static final int AUDIO_STATE_CLOSED = 5;

    public static final int COLOR_ACTIVE = 0xFFD0D0A0;
    public static final int COLOR_IDLE = 0xFFD0D0D0;

    // Pass the activity index to native so it can know how to respond to the start and stop calls.
    // WARNING - must match definitions in NativeAudioContext.h ActivityType
    public static final int ACTIVITY_TEST_OUTPUT = 0;
    public static final int ACTIVITY_TEST_INPUT = 1;
    public static final int ACTIVITY_TAP_TO_TONE = 2;
    public static final int ACTIVITY_RECORD_PLAY = 3;
    public static final int ACTIVITY_ECHO = 4;
    public static final int ACTIVITY_RT_LATENCY = 5;
    public static final int ACTIVITY_GLITCHES = 6;
    public static final int ACTIVITY_TEST_DISCONNECT = 7;
    public static final int ACTIVITY_DATA_PATHS = 8;

    private int mAudioState = AUDIO_STATE_CLOSED;
    protected String audioManagerSampleRate;
    protected int audioManagerFramesPerBurst;
    protected ArrayList<StreamContext> mStreamContexts;
    private Button mOpenButton;
    private Button mStartButton;
    private Button mPauseButton;
    private Button mStopButton;
    private Button mCloseButton;
    private MyStreamSniffer mStreamSniffer;
    private CheckBox mCallbackReturnStopBox;
    private int mSampleRate;
    private int mSingleTestIndex = -1;
    private static boolean mBackgroundEnabled;

    public String getTestName() {
        return "TestAudio";
    }

    public static class StreamContext {
        StreamConfigurationView configurationView;
        AudioStreamTester tester;

        boolean isInput() {
            return tester.getCurrentAudioStream().isInput();
        }
    }

    // Periodically query the status of the streams.
    protected class MyStreamSniffer {
        public static final int SNIFFER_UPDATE_PERIOD_MSEC = 150;
        public static final int SNIFFER_UPDATE_DELAY_MSEC = 300;

        private Handler mHandler;

        // Display status info for the stream.
        private Runnable runnableCode = new Runnable() {
            @Override
            public void run() {
                boolean streamClosed = false;
                boolean gotViews = false;
                for (StreamContext streamContext : mStreamContexts) {
                    AudioStreamBase.StreamStatus status = streamContext.tester.getCurrentAudioStream().getStreamStatus();
                    AudioStreamBase.DoubleStatistics latencyStatistics =
                            streamContext.tester.getCurrentAudioStream().getLatencyStatistics();
                    if (streamContext.configurationView != null) {
                        // Handler runs this on the main UI thread.
                        int framesPerBurst = streamContext.tester.getCurrentAudioStream().getFramesPerBurst();
                        status.framesPerCallback = getFramesPerCallback();
                        String msg = "";
                        msg += "timestamp.latency = " + latencyStatistics.dump() + "\n";
                        msg += status.dump(framesPerBurst);
                        streamContext.configurationView.setStatusText(msg);
                        updateStreamDisplay();
                        gotViews = true;
                    }

                    streamClosed = streamClosed || (status.state >= 12);
                }

                if (streamClosed) {
                    onStreamClosed();
                } else {
                    // Repeat this runnable code block again.
                    if (gotViews) {
                        mHandler.postDelayed(runnableCode, SNIFFER_UPDATE_PERIOD_MSEC);
                    }
                }
            }
        };

        private void startStreamSniffer() {
            stopStreamSniffer();
            mHandler = new Handler(Looper.getMainLooper());
            // Start the initial runnable task by posting through the handler
            mHandler.postDelayed(runnableCode, SNIFFER_UPDATE_DELAY_MSEC);
        }

        private void stopStreamSniffer() {
            if (mHandler != null) {
                mHandler.removeCallbacks(runnableCode);
            }
        }

    }

    public static void setBackgroundEnabled(boolean enabled) {
        mBackgroundEnabled = enabled;
    }
    public static boolean isBackgroundEnabled() {
        return mBackgroundEnabled;
    }

    public void onStreamClosed() {
    }

    protected abstract void inflateActivity();

    void updateStreamDisplay() {
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        inflateActivity();
        findAudioCommon();
    }

    public void hideSettingsViews() {
        for (StreamContext streamContext : mStreamContexts) {
            if (streamContext.configurationView != null) {
                streamContext.configurationView.hideSettingsView();
            }
        }
    }

    abstract int getActivityType();

    public void setSingleTestIndex(int testIndex) {
        mSingleTestIndex = testIndex;
    }
    public int getSingleTestIndex() {
        return mSingleTestIndex;
    }

    @Override
    protected void onStart() {
        super.onStart();
        resetConfiguration();
        setActivityType(getActivityType());
    }

    protected void resetConfiguration() {
    }

    @Override
    protected void onStop() {
        if (!isBackgroundEnabled()) {
            Log.i(TAG, "onStop() called so stop the test =========================");
            onStopTest();
        }
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        if (isBackgroundEnabled()) {
            Log.i(TAG, "onDestroy() called so stop the test =========================");
            onStopTest();
        }
        mAudioState = AUDIO_STATE_CLOSED;
        super.onDestroy();
    }

    protected void updateEnabledWidgets() {
        if (mOpenButton != null) {
            mOpenButton.setBackgroundColor(mAudioState == AUDIO_STATE_OPEN ? COLOR_ACTIVE : COLOR_IDLE);
            mStartButton.setBackgroundColor(mAudioState == AUDIO_STATE_STARTED ? COLOR_ACTIVE : COLOR_IDLE);
            mPauseButton.setBackgroundColor(mAudioState == AUDIO_STATE_PAUSED ? COLOR_ACTIVE : COLOR_IDLE);
            mStopButton.setBackgroundColor(mAudioState == AUDIO_STATE_STOPPED ? COLOR_ACTIVE : COLOR_IDLE);
            mCloseButton.setBackgroundColor(mAudioState == AUDIO_STATE_CLOSED ? COLOR_ACTIVE : COLOR_IDLE);
        }
        setConfigViewsEnabled(mAudioState == AUDIO_STATE_CLOSED);
    }

    private void setConfigViewsEnabled(boolean b) {
        for (StreamContext streamContext : mStreamContexts) {
            if (streamContext.configurationView != null) {
                streamContext.configurationView.setChildrenEnabled(b);
            }
        }
    }
    private void applyConfigurationViewsToModels() {
        for (StreamContext streamContext : mStreamContexts) {
            if (streamContext.configurationView != null) {
                streamContext.configurationView.applyToModel(streamContext.tester.requestedConfiguration);
            }
        }
    }

    abstract boolean isOutput();

    public void clearStreamContexts() {
        mStreamContexts.clear();
    }

    public StreamContext addOutputStreamContext() {
        StreamContext streamContext = new StreamContext();
        streamContext.tester = AudioOutputTester.getInstance();
        streamContext.configurationView = (StreamConfigurationView)
                findViewById(R.id.outputStreamConfiguration);
        if (streamContext.configurationView == null) {
            streamContext.configurationView = (StreamConfigurationView)
                    findViewById(R.id.streamConfiguration);
        }
        if (streamContext.configurationView != null) {
            streamContext.configurationView.setOutput(true);
        }
        mStreamContexts.add(streamContext);
        return streamContext;
    }

    public AudioOutputTester addAudioOutputTester() {
        StreamContext streamContext = addOutputStreamContext();
        return (AudioOutputTester) streamContext.tester;
    }

    public StreamContext addInputStreamContext() {
        StreamContext streamContext = new StreamContext();
        streamContext.tester = AudioInputTester.getInstance();
        streamContext.configurationView = (StreamConfigurationView)
                findViewById(R.id.inputStreamConfiguration);
        if (streamContext.configurationView == null) {
            streamContext.configurationView = (StreamConfigurationView)
                    findViewById(R.id.streamConfiguration);
        }
        if (streamContext.configurationView != null) {
            streamContext.configurationView.setOutput(false);
        }
        streamContext.tester = AudioInputTester.getInstance();
        mStreamContexts.add(streamContext);
        return streamContext;
    }

    public AudioInputTester addAudioInputTester() {
        StreamContext streamContext = addInputStreamContext();
        return (AudioInputTester) streamContext.tester;
    }

    void updateStreamConfigurationViews() {
        for (StreamContext streamContext : mStreamContexts) {
            if (streamContext.configurationView != null) {
                streamContext.configurationView.updateDisplay(streamContext.tester.actualConfiguration);
            }
        }
    }

    StreamContext getFirstInputStreamContext() {
        for (StreamContext streamContext : mStreamContexts) {
            if (streamContext.isInput())
                return streamContext;
        }
        return null;
    }

    StreamContext getFirstOutputStreamContext() {
        for (StreamContext streamContext : mStreamContexts) {
            if (!streamContext.isInput())
                return streamContext;
        }
        return null;
    }

    protected void findAudioCommon() {
        mOpenButton = (Button) findViewById(R.id.button_open);
        if (mOpenButton != null) {
            mStartButton = (Button) findViewById(R.id.button_start);
            mPauseButton = (Button) findViewById(R.id.button_pause);
            mStopButton = (Button) findViewById(R.id.button_stop);
            mCloseButton = (Button) findViewById(R.id.button_close);
        }
        mStreamContexts = new ArrayList<StreamContext>();

        queryNativeAudioParameters();

        mCallbackReturnStopBox = (CheckBox) findViewById(R.id.callbackReturnStop);
        if (mCallbackReturnStopBox != null) {
            mCallbackReturnStopBox.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    OboeAudioStream.setCallbackReturnStop(mCallbackReturnStopBox.isChecked());
                }
            });
        }
        OboeAudioStream.setCallbackReturnStop(false);

        mStreamSniffer = new MyStreamSniffer();
    }

    private void queryNativeAudioParameters() {
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        audioManagerSampleRate = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        String audioManagerFramesPerBurstText = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        audioManagerFramesPerBurst = Integer.parseInt(audioManagerFramesPerBurstText);
    }

    abstract public void setupEffects(int sessionId);

    protected void showErrorToast(String message) {
        showToast("Error: " + message);
    }

    protected void showToast(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(TestAudioActivity.this,
                        message,
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    public void openAudio(View view) {
        try {
            openAudio();
        } catch (Exception e) {
            showErrorToast(e.getMessage());
        }
    }

    public void startAudio(View view) {
        Log.i(TAG, "startAudio() called =======================================");
        try {
            startAudio();
        } catch (Exception e) {
            showErrorToast(e.getMessage());
        }
        keepScreenOn(true);
    }

    protected void keepScreenOn(boolean on) {
        if (on) {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } else {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

    public void stopAudio(View view) {
        stopAudio();
        keepScreenOn(false);
    }

    public void pauseAudio(View view) {
        pauseAudio();
        keepScreenOn(false);
    }

    public void closeAudio(View view) {
        closeAudio();
    }

    public int getSampleRate() {
        return mSampleRate;
    }

    public boolean isTestConfiguredUsingBundle() {
        return false;
    }

    public void openAudio() throws IOException {
        closeAudio();

        if (!isTestConfiguredUsingBundle()) {
            applyConfigurationViewsToModels();
        }

        int sampleRate = 0;

        // Open output streams then open input streams.
        // This is so that the capacity of input stream can be expanded to
        // match the burst size of the output for full duplex.
        for (StreamContext streamContext : mStreamContexts) {
            if (!streamContext.isInput()) {
                openStreamContext(streamContext);
                int streamSampleRate = streamContext.tester.actualConfiguration.getSampleRate();
                if (sampleRate == 0) {
                    sampleRate = streamSampleRate;
                }
            }
        }
        for (StreamContext streamContext : mStreamContexts) {
            if (streamContext.isInput()) {
                if (sampleRate != 0) {
                    streamContext.tester.requestedConfiguration.setSampleRate(sampleRate);
                }
                openStreamContext(streamContext);
            }
        }
        updateEnabledWidgets();
        mStreamSniffer.startStreamSniffer();
    }

    /**
     * @param deviceId
     * @return true if the device is TYPE_BLUETOOTH_SCO
     */
    boolean isScoDevice(int deviceId) {
        if (deviceId == 0) return false; // Unspecified
        AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        final AudioDeviceInfo[] devices = audioManager.getDevices(AudioManager.GET_DEVICES_ALL);
        for (AudioDeviceInfo device : devices) {
            if (device.getId() == deviceId) {
                return device.getType() == AudioDeviceInfo.TYPE_BLUETOOTH_SCO;
            }
        }
        return false;
    }

    private void openStreamContext(StreamContext streamContext) throws IOException {
        StreamConfiguration requestedConfig = streamContext.tester.requestedConfiguration;
        StreamConfiguration actualConfig = streamContext.tester.actualConfiguration;
        requestedConfig.setFramesPerBurst(audioManagerFramesPerBurst);

        streamContext.tester.open(); // OPEN the stream

        mSampleRate = actualConfig.getSampleRate();
        mAudioState = AUDIO_STATE_OPEN;
        int sessionId = actualConfig.getSessionId();
        if (sessionId > 0) {
            setupEffects(sessionId);
        }
        if (streamContext.configurationView != null) {
            streamContext.configurationView.updateDisplay(streamContext.tester.actualConfiguration);
        }
    }

    // Native methods
    private native int startNative();
    private native int pauseNative();
    private native int stopNative();
    protected native void setActivityType(int activityType);
    private native int getFramesPerCallback();

    public void startAudio() throws IOException {
        int result = startNative();
        if (result < 0) {
            showErrorToast("Start failed with " + result);
            throw new IOException("startNative returned " + result);
        } else {
            for (StreamContext streamContext : mStreamContexts) {
                StreamConfigurationView configView = streamContext.configurationView;
                if (configView != null) {
                    configView.updateDisplay(streamContext.tester.actualConfiguration);
                }
            }
            mAudioState = AUDIO_STATE_STARTED;
            updateEnabledWidgets();
        }
    }

    protected void toastPauseError(int result) {
        showErrorToast("Pause failed with " + result);
    }

    public void pauseAudio() {
        int result = pauseNative();
        if (result < 0) {
            toastPauseError(result);
        } else {
            mAudioState = AUDIO_STATE_PAUSED;
            updateEnabledWidgets();
        }
    }

    public void stopAudio() {
        int result = stopNative();
        if (result < 0) {
            showErrorToast("Stop failed with " + result);
        } else {
            mAudioState = AUDIO_STATE_STOPPED;
            updateEnabledWidgets();
        }
    }

    public void runTest() {}

    // This should only be called from UI events such as onStop or a button press.
    public void onStopTest() {
        stopTest();
    }

    public void stopTest() {
        stopAudio();
        closeAudio();
    }

    public void stopAudioQuiet() {
        stopNative();
        mAudioState = AUDIO_STATE_STOPPED;
        updateEnabledWidgets();
    }

    // Make synchronized so we don't close from two streams at the same time.
    public synchronized void closeAudio() {
        if (mAudioState >= AUDIO_STATE_CLOSING) {
            Log.d(TAG, "closeAudio() already closing");
            return;
        }
        mAudioState = AUDIO_STATE_CLOSING;

        mStreamSniffer.stopStreamSniffer();
        // Close output streams first because legacy callbacks may still be active
        // and an output stream may be calling the input stream.
        for (StreamContext streamContext : mStreamContexts) {
            if (!streamContext.isInput()) {
                streamContext.tester.close();
            }
        }
        for (StreamContext streamContext : mStreamContexts) {
            if (streamContext.isInput()) {
                streamContext.tester.close();
            }
        }

        mAudioState = AUDIO_STATE_CLOSED;
        updateEnabledWidgets();
    }

    void startBluetoothSco() {
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        myAudioMgr.startBluetoothSco();
    }

    void stopBluetoothSco() {
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        myAudioMgr.stopBluetoothSco();
    }

}
