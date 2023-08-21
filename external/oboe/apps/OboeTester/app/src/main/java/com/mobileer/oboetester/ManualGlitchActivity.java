/*
 * Copyright 2019 The Android Open Source Project
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

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.widget.SeekBar;
import android.widget.TextView;

import java.io.IOException;

public class ManualGlitchActivity extends GlitchActivity {

    public static final String KEY_IN_PRESET = "in_preset";

    public static final String KEY_DURATION = "duration";
    public static final int VALUE_DEFAULT_DURATION = 10;

    public static final String KEY_BUFFER_BURSTS = "buffer_bursts";
    public static final int VALUE_DEFAULT_BUFFER_BURSTS = 2;

    public static final String KEY_TOLERANCE = "tolerance";
    private static final float DEFAULT_TOLERANCE = 0.1f;

    private TextView mTextTolerance;
    private SeekBar mFaderTolerance;
    protected ExponentialTaper mTaperTolerance;
    private WaveformView mWaveformView;
    private float[] mWaveform = new float[256];
    private boolean mTestRunningByIntent;
    private Bundle mBundleFromIntent;

    private float   mTolerance = DEFAULT_TOLERANCE;

    private SeekBar.OnSeekBarChangeListener mToleranceListener = new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            setToleranceProgress(progress);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
        }
    };

    protected void setToleranceProgress(int progress) {
        float tolerance = (float) mTaperTolerance.linearToExponential(
                ((double)progress) / FADER_PROGRESS_MAX);
        setTolerance(tolerance);
        mTextTolerance.setText("Tolerance = " + String.format("%5.3f", tolerance));
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mBundleFromIntent = getIntent().getExtras();

        mTextTolerance = (TextView) findViewById(R.id.textTolerance);
        mFaderTolerance = (SeekBar) findViewById(R.id.faderTolerance);
        mTaperTolerance = new ExponentialTaper(0.0, 0.5, 100.0);
        mFaderTolerance.setOnSeekBarChangeListener(mToleranceListener);
        setToleranceFader(DEFAULT_TOLERANCE);

        mWaveformView = (WaveformView) findViewById(R.id.waveview_audio);
    }

    private void setToleranceFader(float tolerance) {
        int progress = (int) Math.round((mTaperTolerance.exponentialToLinear(
                tolerance) * FADER_PROGRESS_MAX));
        mFaderTolerance.setProgress(progress);
    }

    @Override
    protected void inflateActivity() {
        setContentView(R.layout.activity_manual_glitches);
    }

    @Override
    public void onResume(){
        super.onResume();
        processBundleFromIntent();
    }

    @Override
    public void onNewIntent(Intent intent) {
        mBundleFromIntent = intent.getExtras();
    }

    private void processBundleFromIntent() {
        if (mBundleFromIntent == null) {
            return;
        }
        if (mTestRunningByIntent) {
            return;
        }

        mResultFileName = null;
        if (mBundleFromIntent.containsKey(KEY_FILE_NAME)) {
            mTestRunningByIntent = true;
            mResultFileName = mBundleFromIntent.getString(KEY_FILE_NAME);

            // Delay the test start to avoid race conditions.
            Handler handler = new Handler(Looper.getMainLooper()); // UI thread
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    startAutomaticTest();
                }
            }, 500); // TODO where is the race, close->open?

        }
    }

    @Override
    public boolean isTestConfiguredUsingBundle() {
        return mBundleFromIntent != null;
    }

    void configureStreamsFromBundle(Bundle bundle) {
        // Extract common parameters
        super.configureStreamsFromBundle(bundle);

        StreamConfiguration requestedInConfig = mAudioInputTester.requestedConfiguration;
        StreamConfiguration requestedOutConfig = mAudioOutTester.requestedConfiguration;

        // Extract custom parameters from the bundle.
        float tolerance = bundle.getFloat(KEY_TOLERANCE, DEFAULT_TOLERANCE);
        setToleranceFader(tolerance);
        setTolerance(tolerance);
        mTolerance = tolerance;

        String defaultText = StreamConfiguration.convertInputPresetToText(
                StreamConfiguration.INPUT_PRESET_VOICE_RECOGNITION);
        String text = bundle.getString(KEY_IN_PRESET, defaultText);
        int inputPreset = StreamConfiguration.convertTextToInputPreset(text);
        requestedInConfig.setInputPreset(inputPreset);
    }

    public void startAudioTest() throws IOException {
        super.startAudioTest();
        setToleranceProgress(mFaderTolerance.getProgress());
    }

    void startAutomaticTest() {
        configureStreamsFromBundle(mBundleFromIntent);

        int durationSeconds = mBundleFromIntent.getInt(KEY_DURATION, VALUE_DEFAULT_DURATION);
        int numBursts = mBundleFromIntent.getInt(KEY_BUFFER_BURSTS, VALUE_DEFAULT_BUFFER_BURSTS);

        try {
            onStartAudioTest(null);
            int sizeFrames = mAudioOutTester.getCurrentAudioStream().getFramesPerBurst() * numBursts;
            mAudioOutTester.getCurrentAudioStream().setBufferSizeInFrames(sizeFrames);

            // Schedule the end of the test.
            Handler handler = new Handler(Looper.getMainLooper()); // UI thread
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    stopAutomaticTest();
                }
            }, durationSeconds * 1000);
        } catch (IOException e) {
            String report = "Open failed: " + e.getMessage();
            maybeWriteTestResult(report);
            mTestRunningByIntent = false;
        } finally {
            mBundleFromIntent = null;
        }

    }

    void stopAutomaticTest() {
        String report = getCommonTestReport()
                + String.format("tolerance = %5.3f\n", mTolerance)
                + mLastGlitchReport;
        onStopAudioTest(null);
        maybeWriteTestResult(report);
        mTestRunningByIntent = false;
    }

    // Only call from UI thread.
    @Override
    public void onTestFinished() {
        super.onTestFinished();
    }
    // Only call from UI thread.
    @Override
    public void onTestBegan() {
        mWaveformView.clearSampleData();
        mWaveformView.postInvalidate();
        super.onTestBegan();
    }

    // Called on UI thread
    @Override
    protected void onGlitchDetected() {
        int numSamples = getGlitch(mWaveform);
        mWaveformView.setSampleData(mWaveform, 0, numSamples);
        mWaveformView.postInvalidate();
    }

    private float[] getGlitchWaveform() {
        return mWaveform;
    }

    private native int getGlitch(float[] mWaveform);

}
