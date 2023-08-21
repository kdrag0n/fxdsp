/*
 * Copyright 2018 The Android Open Source Project
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
import android.support.annotation.NonNull;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.IOException;
import java.util.ArrayList;

/**
 * Activity to measure latency on a full duplex stream.
 */
public class RoundTripLatencyActivity extends AnalyzerActivity {

    // STATEs defined in LatencyAnalyzer.h
    private static final int STATE_MEASURE_BACKGROUND = 0;
    private static final int STATE_IN_PULSE = 1;
    private static final int STATE_GOT_DATA = 2;
    private final static String LATENCY_FORMAT = "%4.2f";
    // When I use 5.3g I only get one digit after the decimal point!
    private final static String CONFIDENCE_FORMAT = "%5.3f";

    private TextView mAnalyzerView;
    private Button   mMeasureButton;
    private Button   mAverageButton;
    private Button   mCancelButton;
    private Button   mShareButton;
    private boolean  mHasRecording = false;

    private boolean mTestRunningByIntent;
    private Bundle  mBundleFromIntent;
    private int     mBufferBursts = -1;
    private Handler mHandler = new Handler(Looper.getMainLooper()); // UI thread

    // Run the test several times and report the acverage latency.
    protected class LatencyAverager {
        private final static int AVERAGE_TEST_DELAY_MSEC = 1000; // arbitrary
        private static final int GOOD_RUNS_REQUIRED = 5; // arbitrary
        private static final int MAX_BAD_RUNS_ALLOWED = 5; // arbitrary
        private int mBadCount = 0; // number of bad measurements
        private int mGoodCount = 0; // number of good measurements

        ArrayList<Double> mLatencies = new ArrayList<Double>(GOOD_RUNS_REQUIRED);
        ArrayList<Double> mConfidences = new ArrayList<Double>(GOOD_RUNS_REQUIRED);
        private double  mLatencyMin;
        private double  mLatencyMax;
        private double  mConfidenceSum;
        private boolean mActive;
        private String  mLastReport = "";

        // Called on UI thread.
        String onAnalyserDone() {
            String message;
            boolean reschedule = false;
            if (!mActive) {
                message = "";
            } else if (getMeasuredResult() != 0) {
                mBadCount++;
                if (mBadCount > MAX_BAD_RUNS_ALLOWED) {
                    cancel();
                    updateButtons(false);
                    message = "averaging cancelled due to error\n";
                } else {
                    message = "skipping this bad run, "
                            + mBadCount + " of " + MAX_BAD_RUNS_ALLOWED + " max\n";
                    reschedule = true;
                }
            } else {
                mGoodCount++;
                double latency = getMeasuredLatencyMillis();
                double confidence = getMeasuredConfidence();
                mLatencies.add(latency);
                mConfidences.add(confidence);
                mConfidenceSum += confidence;
                mLatencyMin = Math.min(mLatencyMin, latency);
                mLatencyMax = Math.max(mLatencyMax, latency);
                if (mGoodCount < GOOD_RUNS_REQUIRED) {
                    reschedule = true;
                } else {
                    mActive = false;
                    updateButtons(false);
                }
                message = reportAverage();
            }
            if (reschedule) {
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        measureSingleLatency();
                    }
                }, AVERAGE_TEST_DELAY_MSEC);
            }
            return message;
        }

        private String reportAverage() {
            String message;
            if (mGoodCount == 0 || mConfidenceSum == 0.0) {
                message = "num.iterations = " + mGoodCount + "\n";
            } else {
                final double mAverageConfidence = mConfidenceSum / mGoodCount;
                double meanLatency = calculateMeanLatency();
                double meanAbsoluteDeviation = calculateMeanAbsoluteDeviation(meanLatency);
                message = "average.latency.msec = " + String.format(LATENCY_FORMAT, meanLatency) + "\n"
                        + "mean.absolute.deviation = " + String.format(LATENCY_FORMAT, meanAbsoluteDeviation) + "\n"
                        + "average.confidence = " + String.format(CONFIDENCE_FORMAT, mAverageConfidence) + "\n"
                        + "min.latency.msec = " + String.format(LATENCY_FORMAT, mLatencyMin) + "\n"
                        + "max.latency.msec = " + String.format(LATENCY_FORMAT, mLatencyMax) + "\n"
                        + "num.iterations = " + mGoodCount + "\n";
            }
            message += "num.failed = " + mBadCount + "\n";
            mLastReport = message;
            return message;
        }

        private double calculateMeanAbsoluteDeviation(double meanLatency) {
            double deviationSum = 0.0;
            for (double latency : mLatencies) {
                deviationSum += Math.abs(latency - meanLatency);
            }
            return deviationSum / mLatencies.size();
        }

        private double calculateMeanLatency() {
            double latencySum = 0.0;
            for (double latency : mLatencies) {
                latencySum += latency;
            }
            return latencySum / mLatencies.size();
        }

        // Called on UI thread.
        public void start() {
            mLatencies.clear();
            mConfidences.clear();
            mConfidenceSum = 0.0;
            mLatencyMax = Double.MIN_VALUE;
            mLatencyMin = Double.MAX_VALUE;
            mBadCount = 0;
            mGoodCount = 0;
            mActive = true;
            mLastReport = "";
            measureSingleLatency();
        }

        public void clear() {
            mActive = false;
            mLastReport = "";
        }

        public void cancel() {
            mActive = false;
        }

        public boolean isActive() {
            return mActive;
        }

        public String getLastReport() {
            return mLastReport;
        }
    }
    LatencyAverager mLatencyAverager = new LatencyAverager();

    // Periodically query the status of the stream.
    protected class LatencySniffer {
        private int counter = 0;
        public static final int SNIFFER_UPDATE_PERIOD_MSEC = 150;
        public static final int SNIFFER_UPDATE_DELAY_MSEC = 300;

        // Display status info for the stream.
        private Runnable runnableCode = new Runnable() {
            @Override
            public void run() {
                String message;

                if (isAnalyzerDone()) {
                    message = mLatencyAverager.onAnalyserDone();
                    message += onAnalyzerDone();
                } else {
                    message = getProgressText();
                    message += "please wait... " + counter + "\n";
                    message += convertStateToString(getAnalyzerState());

                    // Repeat this runnable code block again.
                    mHandler.postDelayed(runnableCode, SNIFFER_UPDATE_PERIOD_MSEC);
                }
                setAnalyzerText(message);
                counter++;
            }
        };

        private void startSniffer() {
            counter = 0;
            // Start the initial runnable task by posting through the handler
            mHandler.postDelayed(runnableCode, SNIFFER_UPDATE_DELAY_MSEC);
        }

        private void stopSniffer() {
            if (mHandler != null) {
                mHandler.removeCallbacks(runnableCode);
            }
        }
    }

    static String convertStateToString(int state) {
        switch (state) {
            case STATE_MEASURE_BACKGROUND: return "BACKGROUND";
            case STATE_IN_PULSE: return "RECORDING";
            case STATE_GOT_DATA: return "ANALYZING";
            default: return "DONE";
        }
    }

    private String getProgressText() {
        int progress = getAnalyzerProgress();
        int state = getAnalyzerState();
        int resetCount = getResetCount();
        String message = String.format("progress = %d\nstate = %d\n#resets = %d\n",
                progress, state, resetCount);
        message += mLatencyAverager.getLastReport();
        return message;
    }

    private String onAnalyzerDone() {
        String message = getResultString();
        if (mTestRunningByIntent) {
            String report = getCommonTestReport();
            report += message;
            maybeWriteTestResult(report);
        }
        mTestRunningByIntent = false;
        mHasRecording = true;
        stopAudioTest();
        return message;
    }

    @NonNull
    private String getResultString() {
        int result = getMeasuredResult();
        int resetCount = getResetCount();
        double confidence = getMeasuredConfidence();
        String message = "";

        message += String.format("confidence = " + CONFIDENCE_FORMAT + "\n", confidence);
        message += String.format("result.text = %s\n", resultCodeToString(result));

        // Only report valid latencies.
        if (result == 0) {
            int latencyFrames = getMeasuredLatency();
            double latencyMillis = getMeasuredLatencyMillis();
            int bufferSize = mAudioOutTester.getCurrentAudioStream().getBufferSizeInFrames();
            int latencyEmptyFrames = latencyFrames - bufferSize;
            double latencyEmptyMillis = latencyEmptyFrames * 1000.0 / getSampleRate();
            message += String.format("latency.msec = " + LATENCY_FORMAT + "\n", latencyMillis);
            message += String.format("latency.frames = %d\n", latencyFrames);
            message += String.format("latency.empty.msec = " + LATENCY_FORMAT + "\n", latencyEmptyMillis);
            message += String.format("latency.empty.frames = %d\n", latencyEmptyFrames);
        }

        message += String.format("rms.signal = %7.5f\n", getSignalRMS());
        message += String.format("rms.noise = %7.5f\n", getBackgroundRMS());
        message += String.format("reset.count = %d\n", resetCount);
        message += String.format("result = %d\n", result);

        return message;
    }

    private LatencySniffer mLatencySniffer = new LatencySniffer();

    native int getAnalyzerProgress();
    native int getMeasuredLatency();
    double getMeasuredLatencyMillis() {
        return getMeasuredLatency() * 1000.0 / getSampleRate();
    }
    native double getMeasuredConfidence();
    native double getBackgroundRMS();
    native double getSignalRMS();

    private void setAnalyzerText(String s) {
        mAnalyzerView.setText(s);
    }

    @Override
    protected void inflateActivity() {
        setContentView(R.layout.activity_rt_latency);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mMeasureButton = (Button) findViewById(R.id.button_measure);
        mAverageButton = (Button) findViewById(R.id.button_average);
        mCancelButton = (Button) findViewById(R.id.button_cancel);
        mShareButton = (Button) findViewById(R.id.button_share);
        mShareButton.setEnabled(false);
        mAnalyzerView = (TextView) findViewById(R.id.text_status);
        mAnalyzerView.setMovementMethod(new ScrollingMovementMethod());
        updateEnabledWidgets();

        hideSettingsViews();

        mBufferSizeView.setFaderNormalizedProgress(0.0); // for lowest latency

        mBundleFromIntent = getIntent().getExtras();
    }

    @Override
    public void onNewIntent(Intent intent) {
        mBundleFromIntent = intent.getExtras();
    }

    @Override
    int getActivityType() {
        return ACTIVITY_RT_LATENCY;
    }

    @Override
    protected void onStart() {
        super.onStart();
        mHasRecording = false;
        updateButtons(false);
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
            getFirstInputStreamContext().configurationView.setExclusiveMode(true);
            getFirstOutputStreamContext().configurationView.setExclusiveMode(true);
            mBufferBursts = mBundleFromIntent.getInt(KEY_BUFFER_BURSTS, mBufferBursts);

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

    void startAutomaticTest() {
        try {
            configureStreamsFromBundle(mBundleFromIntent);
            onMeasure(null);
        } finally {
            mBundleFromIntent = null;
        }
    }

    @Override
    public void onResume(){
        super.onResume();
        processBundleFromIntent();
    }

    @Override
    protected void onStop() {
        mLatencySniffer.stopSniffer();
        super.onStop();
    }

    public void onMeasure(View view) {
        mLatencyAverager.clear();
        measureSingleLatency();
    }

    void updateButtons(boolean running) {
        boolean busy = running || mLatencyAverager.isActive();
        mMeasureButton.setEnabled(!busy);
        mAverageButton.setEnabled(!busy);
        mCancelButton.setEnabled(running);
        mShareButton.setEnabled(!busy && mHasRecording);
    }

    private void measureSingleLatency() {
        try {
            openAudio();
            if (mBufferBursts >= 0) {
                AudioStreamBase stream = mAudioOutTester.getCurrentAudioStream();
                int framesPerBurst = stream.getFramesPerBurst();
                stream.setBufferSizeInFrames(framesPerBurst * mBufferBursts);
                // override buffer size fader
                mBufferSizeView.setEnabled(false);
                mBufferBursts = -1;
            }
            startAudio();
            mLatencySniffer.startSniffer();
            updateButtons(true);
        } catch (IOException e) {
            showErrorToast(e.getMessage());
        }
    }

    public void onAverage(View view) {
        mLatencyAverager.start();
    }

    public void onCancel(View view) {
        mLatencyAverager.cancel();
        stopAudioTest();
    }

    // Call on UI thread
    public void stopAudioTest() {
        mLatencySniffer.stopSniffer();
        stopAudio();
        closeAudio();
        updateButtons(false);
    }

    @Override
    String getWaveTag() {
        return "rtlatency";
    }

    @Override
    boolean isOutput() {
        return false;
    }

    @Override
    public void setupEffects(int sessionId) {
    }
}
