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
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Point;
import android.media.AudioManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.widget.AdapterView;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Select various Audio tests.
 */

public class MainActivity extends Activity {

    private static final String KEY_TEST_NAME = "test";
    public static final String VALUE_TEST_NAME_LATENCY = "latency";
    public static final String VALUE_TEST_NAME_GLITCH = "glitch";

    static {
        // Must match name in CMakeLists.txt
        System.loadLibrary("oboetester");
    }

    private Spinner mModeSpinner;
    private TextView mCallbackSizeEditor;
    protected TextView mDeviceView;
    private TextView mVersionTextView;
    private TextView mBuildTextView;
    private TextView mBluetoothScoStatusView;
    private Bundle mBundleFromIntent;
    private BroadcastReceiver mScoStateReceiver;
    private CheckBox mWorkaroundsCheckBox;
    private CheckBox mBackgroundCheckBox;
    private static String mVersionText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        logScreenSize();

        mVersionTextView = (TextView) findViewById(R.id.versionText);
        mCallbackSizeEditor = (TextView) findViewById(R.id.callbackSize);

        mDeviceView = (TextView) findViewById(R.id.deviceView);
        updateNativeAudioUI();

        // Set mode, eg. MODE_IN_COMMUNICATION
        mModeSpinner = (Spinner) findViewById(R.id.spinnerAudioMode);
        // Update AudioManager now in case user is trying to affect a different app.
        mModeSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
                long mode = mModeSpinner.getSelectedItemId();
                AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
                myAudioMgr.setMode((int)mode);
            }

            @Override
            public void onNothingSelected(AdapterView<?> adapterView) {
            }
        });

        try {
            PackageInfo pinfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            int oboeVersion = OboeAudioStream.getOboeVersionNumber();
            int oboeMajor = (oboeVersion >> 24) & 0xFF;
            int oboeMinor = (oboeVersion >> 16) & 0xFF;
            int oboePatch = oboeVersion & 0xFF;
            mVersionText = "OboeTester (" + pinfo.versionCode + ") v " + pinfo.versionName
                    + ", Oboe v " + oboeMajor + "." + oboeMinor + "." + oboePatch;
            mVersionTextView.setText(mVersionText);
        } catch (PackageManager.NameNotFoundException e) {
            mVersionTextView.setText(e.getMessage());
        }

        mWorkaroundsCheckBox = (CheckBox) findViewById(R.id.boxEnableWorkarounds);
        // Turn off workarounds so we can test the underlying API bugs.
        mWorkaroundsCheckBox.setChecked(false);
        NativeEngine.setWorkaroundsEnabled(false);

        mBackgroundCheckBox = (CheckBox) findViewById(R.id.boxEnableBackground);

        mBuildTextView = (TextView) findViewById(R.id.text_build_info);
        mBuildTextView.setText(Build.DISPLAY);

        mBluetoothScoStatusView = (TextView) findViewById(R.id.textBluetoothScoStatus);
        mScoStateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                int state = intent.getIntExtra(AudioManager.EXTRA_SCO_AUDIO_STATE, -1);
                if (state == AudioManager.SCO_AUDIO_STATE_CONNECTING) {
                    mBluetoothScoStatusView.setText("CONNECTING");
                } else if (state == AudioManager.SCO_AUDIO_STATE_CONNECTED) {
                    mBluetoothScoStatusView.setText("CONNECTED");
                } else if (state == AudioManager.SCO_AUDIO_STATE_DISCONNECTED) {
                    mBluetoothScoStatusView.setText("DISCONNECTED");
                }
            }
        };

        saveIntentBundleForLaterProcessing(getIntent());
    }

    public static String getVersiontext() {
        return mVersionText;
    }

    private void registerScoStateReceiver() {
        registerReceiver(mScoStateReceiver,
                new IntentFilter(AudioManager.ACTION_SCO_AUDIO_STATE_UPDATED));
    }

    private void unregisterScoStateReceiver() {
        unregisterReceiver(mScoStateReceiver);
    }

    private void logScreenSize() {
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int width = size.x;
        int height = size.y;
        Log.i(TestAudioActivity.TAG, "Screen size = " + size.x + " * " + size.y);
    }

    @Override
    public void onNewIntent(Intent intent) {
        saveIntentBundleForLaterProcessing(intent);
    }

    // This will get processed during onResume.
    private void saveIntentBundleForLaterProcessing(Intent intent) {
        mBundleFromIntent = intent.getExtras();
    }

    private void processBundleFromIntent() {
        if (mBundleFromIntent == null) {
            return;
        }

        if (mBundleFromIntent.containsKey(KEY_TEST_NAME)) {
            String testName = mBundleFromIntent.getString(KEY_TEST_NAME);
            if (VALUE_TEST_NAME_LATENCY.equals(testName)) {
                Intent intent = new Intent(this, RoundTripLatencyActivity.class);
                intent.putExtras(mBundleFromIntent);
                startActivity(intent);
            } else if (VALUE_TEST_NAME_GLITCH.equals(testName)) {
                Intent intent = new Intent(this, ManualGlitchActivity.class);
                intent.putExtras(mBundleFromIntent);
                startActivity(intent);
            }
        }
        mBundleFromIntent = null;
    }

    @Override
    public void onResume(){
        super.onResume();
        mWorkaroundsCheckBox.setChecked(NativeEngine.areWorkaroundsEnabled());
        processBundleFromIntent();
        registerScoStateReceiver();
    }

    @Override
    public void onPause(){
        unregisterScoStateReceiver();
        super.onPause();
    }

    private void updateNativeAudioUI() {
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        String audioManagerSampleRate = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
        String audioManagerFramesPerBurst = myAudioMgr.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        mDeviceView.setText("Java AudioManager: rate = " + audioManagerSampleRate +
                ", burst = " + audioManagerFramesPerBurst);
    }

    public void onLaunchTestOutput(View view) {
        onLaunchTest(TestOutputActivity.class);
    }

    public void onLaunchTestInput(View view) {
        onLaunchTest(TestInputActivity.class);
    }

    public void onLaunchTapToTone(View view) {
        onLaunchTest(TapToToneActivity.class);
    }

    public void onLaunchRecorder(View view) {
        onLaunchTest(RecorderActivity.class);
    }

    public void onLaunchEcho(View view) {
        onLaunchTest(EchoActivity.class);
    }

    public void onLaunchRoundTripLatency(View view) {
        onLaunchTest(RoundTripLatencyActivity.class);
    }

    public void onLaunchManualGlitchTest(View view) {
        onLaunchTest(ManualGlitchActivity.class);
    }

    public void onLaunchAutoGlitchTest(View view) { onLaunchTest(AutomatedGlitchActivity.class); }

    public void onLaunchTestDisconnect(View view) {
        onLaunchTest(TestDisconnectActivity.class);
    }

    public void onLaunchTestDataPaths(View view) {
        onLaunchTest(TestDataPathsActivity.class);
    }

    public void onLaunchTestDeviceReport(View view)  {
        onLaunchTest(DeviceReportActivity.class);
    }

    public void onLaunchExtratests(View view) {
        onLaunchTest(ExtraTestsActivity.class);
    }

    private void applyUserOptions() {
        updateCallbackSize();

        long mode = mModeSpinner.getSelectedItemId();
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        myAudioMgr.setMode((int) mode);

        NativeEngine.setWorkaroundsEnabled(mWorkaroundsCheckBox.isChecked());
        TestAudioActivity.setBackgroundEnabled(mBackgroundCheckBox.isChecked());
    }

    private void onLaunchTest(Class clazz) {
        applyUserOptions();
        Intent intent = new Intent(this, clazz);
        startActivity(intent);
    }

    public void onUseCallbackClicked(View view) {
        CheckBox checkBox = (CheckBox) view;
        OboeAudioStream.setUseCallback(checkBox.isChecked());
    }

    protected void showErrorToast(String message) {
        showToast("Error: " + message);
    }

    protected void showToast(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(MainActivity.this,
                        message,
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void updateCallbackSize() {
        CharSequence chars = mCallbackSizeEditor.getText();
        String text = chars.toString();
        int callbackSize = 0;
        try {
            callbackSize = Integer.parseInt(text);
        } catch (NumberFormatException e) {
            showErrorToast("Badly formated callback size: " + text);
            mCallbackSizeEditor.setText("0");
        }
        OboeAudioStream.setCallbackSize(callbackSize);
    }

    public void onSetSpeakerphoneOn(View view) {
        CheckBox checkBox = (CheckBox) view;
        boolean enabled = checkBox.isChecked();
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        myAudioMgr.setSpeakerphoneOn(enabled);
    }

    public void onStartStopBluetoothSco(View view) {
        CheckBox checkBox = (CheckBox) view;
        AudioManager myAudioMgr = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        if (checkBox.isChecked()) {
            myAudioMgr.startBluetoothSco();
        } else {
            myAudioMgr.stopBluetoothSco();
        }
    }
}
