package com.april.themellaapp.activity;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.DialogFragment;

import com.april.themellaapp.R;
import com.april.themellaapp.databinding.SchedulerActivityBinding;
import com.april.themellaapp.fragment.TimePickerFragment;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

import java.util.Locale;

public class NewTaskActivity extends AppCompatActivity implements View.OnClickListener, OnItemSelectedListener {
    private static final String[] paths = {"Pacific Time", "Mountain Time", "Central Time", "Eastern Time"};

    static String MQTT_HOST = "tcp://167.71.112.166:1883";
    static String USERNAME = "themella";
    static String PASSWORD = "!@#mqtt";

    public String sunOnTime = "", sunOffTime = "", monOnTime = "", monOffTime = "", tueOnTime = "", tueOffTime = "", wedOnTime = "", wedOffTime = "", thurOnTime = "", thurOffTime = "", friOnTime = "", friOffTime = "", satOnTime = "", satOffTime = "", mSunday = "0", mMonday = "0", mTuesday = "0", mWednesday = "0", mThursday = "0", mFriday = "0", mSaturday = "0", totalReps = "";
    public int sunStartHour, sunEndHour, monStartHour, monEndHour, tueStartHour, tueEndHour, wedStartHour, wedEndHour, thuStartHour, thuEndHour, friStartHour, friEndHour, satStartHour, satEndHour, timeDifference;

    String sundayTime = "", mondayTime = "", tuesdayTime = "", wednesdayTime = "", thursdayTime = "", fridayTime = "", saturdayTime = "";
    String devID;
    String subscribeTopic = "/Dev/Status";
    String publishTopic = "/Dev/Schedule";

    SchedulerActivityBinding binding;

    MqttAndroidClient client;
    MqttConnectOptions options = new MqttConnectOptions();

    MemoryPersistence persistence = new MemoryPersistence();
    SharedPreferences schedulePreference;
    SharedPreferences.Editor scheduleEditor;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = SchedulerActivityBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());
        String clientId = MqttClient.generateClientId();
        client = new MqttAndroidClient(this.getApplicationContext(), MQTT_HOST, clientId, persistence);

        Bundle bundle = getIntent().getExtras();
        if (bundle != null) {
            devID = bundle.getString("DevID");
            subscribeTopic = devID + "/Status";
            publishTopic = devID + "/Schedule";
        }

        initXMLComponents();
        initMqtt();
    }

    private void initXMLComponents() {
        Spinner spinner = binding.spinner;
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, paths);

        spinner.setAdapter(adapter);
        spinner.setOnItemSelectedListener(this);

        binding.navigateBack.setOnClickListener(this);
        binding.btnSave.setOnClickListener(this);
        binding.sunday.setOnClickListener(this);
        binding.sunOnTimerTextView.setOnClickListener(this);
        binding.sunOffTimerTextView.setOnClickListener(this);
        binding.monday.setOnClickListener(this);
        binding.monOnTimerTextView.setOnClickListener(this);
        binding.monOffTimerTextView.setOnClickListener(this);
        binding.tuesday.setOnClickListener(this);
        binding.tueOnTimerTextView.setOnClickListener(this);
        binding.tueOffTimerTextView.setOnClickListener(this);
        binding.wednesday.setOnClickListener(this);
        binding.wedOnTimerTextView.setOnClickListener(this);
        binding.wedOffTimerTextView.setOnClickListener(this);
        binding.thursday.setOnClickListener(this);
        binding.thursOnTimerTextView.setOnClickListener(this);
        binding.thursOffTimerTextView.setOnClickListener(this);
        binding.friday.setOnClickListener(this);
        binding.friOnTimerTextView.setOnClickListener(this);
        binding.friOffTimerTextView.setOnClickListener(this);
        binding.saturday.setOnClickListener(this);
        binding.satOnTimerTextView.setOnClickListener(this);
        binding.satOffTimerTextView.setOnClickListener(this);

        schedulePreference = getApplicationContext().getSharedPreferences(devID + "ScheduleDetails", MODE_PRIVATE);

        // Sunday
        if (!schedulePreference.getString("Sunday", "").isEmpty()) {
            binding.sunday.setText(schedulePreference.getString("Sunday", ""));
            if (schedulePreference.getString("Sunday", "").equals("Enabled")) {
                binding.sunday.setBackgroundResource(R.color.grey);
                mSunday = "1";
            } else {
                mSunday = "0";
            }
        }
        if (!schedulePreference.getString("SundayOnTime", "").isEmpty()) {
            binding.sunOnTimerTextView.setText(schedulePreference.getString("SundayOnTime", ""));
            sunOnTime = schedulePreference.getString("SundayOnTime", "");
        }
        if (!schedulePreference.getString("SundayOffTime", "").isEmpty()) {
            binding.sunOffTimerTextView.setText(schedulePreference.getString("SundayOffTime", ""));
            sunOffTime = schedulePreference.getString("SundayOffTime", "");
        }

        // Monday
        if (!schedulePreference.getString("Monday", "").isEmpty()) {
            binding.monday.setText(schedulePreference.getString("Monday", ""));
            if (schedulePreference.getString("Monday", "").equals("Enabled")) {
                binding.monday.setBackgroundResource(R.color.grey);
                mMonday = "1";
            } else {
                mMonday = "0";
            }
        }
        if (!schedulePreference.getString("MondayOnTime", "").isEmpty()) {
            binding.monOnTimerTextView.setText(schedulePreference.getString("MondayOnTime", ""));
            monOnTime = schedulePreference.getString("MondayOnTime", "");
        }
        if (!schedulePreference.getString("MondayOffTime", "").isEmpty()) {
            binding.monOffTimerTextView.setText(schedulePreference.getString("MondayOffTime", ""));
            monOffTime = schedulePreference.getString("MondayOffTime", "");
        }

        // Tuesday
        if (!schedulePreference.getString("Tuesday", "").isEmpty()) {
            binding.tuesday.setText(schedulePreference.getString("Tuesday", ""));
            if (schedulePreference.getString("Tuesday", "").equals("Enabled")) {
                binding.tuesday.setBackgroundResource(R.color.grey);
                mTuesday = "1";
            } else {
                mTuesday = "0";
            }
        }
        if (!schedulePreference.getString("TuesdayOnTime", "").isEmpty()) {
            binding.tueOnTimerTextView.setText(schedulePreference.getString("TuesdayOnTime", ""));
            tueOnTime = schedulePreference.getString("TuesdayOnTime", "");
        }
        if (!schedulePreference.getString("TuesdayOffTime", "").isEmpty()) {
            binding.tueOffTimerTextView.setText(schedulePreference.getString("TuesdayOffTime", ""));
            tueOffTime = schedulePreference.getString("TuesdayOffTime", "");
        }

        // Wednesday
        if (!schedulePreference.getString("Wednesday", "").isEmpty()) {
            binding.wednesday.setText(schedulePreference.getString("Wednesday", ""));
            if (schedulePreference.getString("Wednesday", "").equals("Enable")) {
                binding.wednesday.setBackgroundResource(R.color.grey);
                mWednesday = "1";
            } else {
                mWednesday = "0";
            }
        }
        if (!schedulePreference.getString("WednesdayOnTime", "").isEmpty()) {
            binding.wedOnTimerTextView.setText(schedulePreference.getString("WednesdayOnTime", ""));
            wedOnTime = schedulePreference.getString("WednesdayOnTime", "");
        }
        if (!schedulePreference.getString("WednesdayOffTime", "").isEmpty()) {
            binding.wedOffTimerTextView.setText(schedulePreference.getString("WednesdayOffTime", ""));
            wedOffTime = schedulePreference.getString("WednesdayOffTime", "");
        }

        // Thursday
        if (!schedulePreference.getString("Thursday", "").isEmpty()) {
            binding.thursday.setText(schedulePreference.getString("Thursday", ""));
            if (schedulePreference.getString("Thursday", "").equals("Enabled")) {
                binding.thursday.setBackgroundResource(R.color.grey);
                mThursday = "1";
            } else {
                mThursday = "0";
            }
        }
        if (!schedulePreference.getString("ThursdayOnTime", "").isEmpty()) {
            binding.thursOnTimerTextView.setText(schedulePreference.getString("ThursdayOnTime", ""));
            thurOnTime = schedulePreference.getString("ThursdayOnTime", "");
        }
        if (!schedulePreference.getString("ThursdayOffTime", "").isEmpty()) {
            binding.thursOffTimerTextView.setText(schedulePreference.getString("ThursdayOffTime", ""));
            thurOffTime = schedulePreference.getString("ThursdayOffTime", "");
        }

        // Friday
        if (!schedulePreference.getString("Friday", "").isEmpty()) {
            binding.friday.setText(schedulePreference.getString("Friday", ""));
            if (schedulePreference.getString("Friday", "").equals("Enable")) {
                binding.friday.setBackgroundResource(R.color.grey);
                mFriday = "1";
            } else {
                mFriday = "0";
            }
        }
        if (!schedulePreference.getString("FridayOnTime", "").isEmpty()) {
            binding.friOnTimerTextView.setText(schedulePreference.getString("FridayOnTime", ""));
            friOnTime = schedulePreference.getString("FridayOnTime", "");
        }
        if (!schedulePreference.getString("FridayOffTime", "").isEmpty()) {
            binding.friOffTimerTextView.setText(schedulePreference.getString("FridayOffTime", ""));
            friOffTime = schedulePreference.getString("FridayOffTime", "");
        }

        // Saturday
        if (!schedulePreference.getString("Saturday", "").isEmpty()) {
            binding.saturday.setText(schedulePreference.getString("Saturday", ""));
            if (schedulePreference.getString("Saturday", "").equals("Enable")) {
                binding.saturday.setBackgroundResource(R.color.grey);
                mSaturday = "1";
            } else {
                mSaturday = "0";
            }
        }
        if (!schedulePreference.getString("SaturdayOnTime", "").isEmpty()) {
            binding.satOnTimerTextView.setText(schedulePreference.getString("SaturdayOnTime", ""));
            satOnTime = schedulePreference.getString("SaturdayOnTime", "");
        }
        if (!schedulePreference.getString("SaturdayOffTime", "").isEmpty()) {
            binding.satOffTimerTextView.setText(schedulePreference.getString("SaturdayOffTime", ""));
            satOffTime = schedulePreference.getString("SaturdayOffTime", "");
        }
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onClick(View view) {
        String DIALOG_TIME = "TimePicker";
        if (view == binding.sunday) {
            if (mSunday.equalsIgnoreCase("0")) {
                binding.sunday.setBackgroundResource(R.color.grey);
                binding.sunday.setText("Enabled");
                mSunday = "1";
            } else {
                binding.sunday.setBackgroundResource(R.color.default_grey);
                binding.sunday.setText("Disabled");
                mSunday = "0";
            }
        } else if (view == binding.monday) {
            if (mMonday.equalsIgnoreCase("0")) {
                binding.monday.setBackgroundResource(R.color.grey);
                binding.monday.setText("Enabled");
                mMonday = "1";
            } else {
                binding.monday.setBackgroundResource(R.color.default_grey);
                binding.monday.setText("Disabled");
                mMonday = "0";
            }
        } else if (view == binding.tuesday) {
            if (mTuesday.equalsIgnoreCase("0")) {
                binding.tuesday.setBackgroundResource(R.color.grey);
                binding.tuesday.setText("Enabled");
                mTuesday = "1";
            } else {
                binding.tuesday.setBackgroundResource(R.color.default_grey);
                binding.tuesday.setText("Disabled");
                mTuesday = "0";
            }
        } else if (view == binding.wednesday) {
            if (mWednesday.equalsIgnoreCase("0")) {
                binding.wednesday.setBackgroundResource(R.color.grey);
                binding.wednesday.setText("Enabled");
                mWednesday = "1";
            } else {
                binding.wednesday.setBackgroundResource(R.color.default_grey);
                binding.wednesday.setText("Disabled");
                mWednesday = "0";
            }
        } else if (view == binding.thursday) {
            if (mThursday.equalsIgnoreCase("0")) {
                binding.thursday.setBackgroundResource(R.color.grey);
                binding.thursday.setText("Enabled");
                mThursday = "1";
            } else {
                binding.thursday.setBackgroundResource(R.color.default_grey);
                binding.thursday.setText("Disabled");
                mThursday = "0";
            }
        } else if (view == binding.friday) {
            if (mFriday.equalsIgnoreCase("0")) {
                binding.friday.setBackgroundResource(R.color.grey);
                binding.friday.setText("Enabled");
                mFriday = "1";
            } else {
                binding.friday.setBackgroundResource(R.color.default_grey);
                binding.friday.setText("Disabled");
                mFriday = "0";
            }
        } else if (view == binding.saturday) {
            if (mSaturday.equalsIgnoreCase("0")) {
                binding.saturday.setBackgroundResource(R.color.grey);
                binding.saturday.setText("Enabled");
                mSaturday = "1";
            } else {
                binding.saturday.setBackgroundResource(R.color.default_grey);
                binding.saturday.setText("Disabled");
                mSaturday = "0";
            }
        } else if (view == binding.sunOnTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 1);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.sunOffTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 2);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.monOnTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 3);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.monOffTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 4);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.tueOnTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 5);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.tueOffTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 6);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.wedOnTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 7);
            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.wedOffTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 8);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.thursOnTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 9);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.thursOffTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 10);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.friOnTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 11);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.friOffTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 12);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.satOnTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 13);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.satOffTimerTextView) {
            Bundle bundle = new Bundle();
            bundle.putInt("TIMER", 14);

            DialogFragment newFragment = new TimePickerFragment();
            newFragment.setArguments(bundle);
            newFragment.show(getSupportFragmentManager(), DIALOG_TIME);
        } else if (view == binding.btnSave) {
            if (validateFields()) {
                String packetValue = sundayTime.concat(":").concat(mondayTime).concat(":").concat(tuesdayTime).concat(":").concat(wednesdayTime).concat(":").concat(thursdayTime).concat(":").concat(fridayTime).concat(":").concat(saturdayTime).concat(":").concat(totalReps).concat(String.valueOf(timeDifference));
                Log.i("packetData", packetValue);

                schedulePreference = getApplicationContext().getSharedPreferences(devID + "ScheduleDetails", MODE_PRIVATE);
                scheduleEditor = schedulePreference.edit();
                scheduleEditor.putString("Sunday", binding.sunday.getText().toString());
                scheduleEditor.putString("SundayOnTime", binding.sunOnTimerTextView.getText().toString());
                scheduleEditor.putString("SundayOffTime", binding.sunOffTimerTextView.getText().toString());

                scheduleEditor.putString("Monday", binding.monday.getText().toString());
                scheduleEditor.putString("MondayOnTime", binding.monOnTimerTextView.getText().toString());
                scheduleEditor.putString("MondayOffTime", binding.monOffTimerTextView.getText().toString());

                scheduleEditor.putString("Tuesday", binding.tuesday.getText().toString());
                scheduleEditor.putString("TuesdayOnTime", binding.tueOnTimerTextView.getText().toString());
                scheduleEditor.putString("TuesdayOffTime", binding.tueOffTimerTextView.getText().toString());

                scheduleEditor.putString("Wednesday", binding.wednesday.getText().toString());
                scheduleEditor.putString("WednesdayOnTime", binding.wedOnTimerTextView.getText().toString());
                scheduleEditor.putString("WednesdayOffTime", binding.wedOffTimerTextView.getText().toString());

                scheduleEditor.putString("Thursday", binding.thursday.getText().toString());
                scheduleEditor.putString("ThursdayOnTime", binding.thursOnTimerTextView.getText().toString());
                scheduleEditor.putString("ThursdayOffTime", binding.thursOffTimerTextView.getText().toString());

                scheduleEditor.putString("Friday", binding.friday.getText().toString());
                scheduleEditor.putString("FridayOnTime", binding.friOnTimerTextView.getText().toString());
                scheduleEditor.putString("FridayOffTime", binding.friOffTimerTextView.getText().toString());

                scheduleEditor.putString("Saturday", binding.saturday.getText().toString());
                scheduleEditor.putString("SaturdayOnTime", binding.satOnTimerTextView.getText().toString());
                scheduleEditor.putString("SaturdayOffTime", binding.satOffTimerTextView.getText().toString());

                scheduleEditor.apply();

                String topic = publishTopic;
                try {
                    client.publish(topic, packetValue.getBytes(), 0, false);
                } catch (MqttException e) {
                    // e.printStackTrace();
                }
            }
        } else if (view == binding.navigateBack) {
            finish();
        }
    }

    public void initMqtt() {
        try {
            options.setUserName(USERNAME);
            options.setPassword(PASSWORD.toCharArray());
            options.setAutomaticReconnect(true);
            options.setMqttVersion(MqttConnectOptions.MQTT_VERSION_3_1);
            IMqttToken token = client.connect(options);

            token.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    try {
                        client.subscribe(subscribeTopic, 0);
                    } catch (MqttException e) {
                        // e.printStackTrace();
                    }
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Toast.makeText(NewTaskActivity.this, "Connection Failed", Toast.LENGTH_LONG).show();
                }
            });
        } catch (MqttException e) {
            // e.printStackTrace();
        }

        client.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectComplete(boolean b, String s) {
                if (b) {
                    try {
                        client.subscribe(subscribeTopic, 0);
                    } catch (MqttException e) {
                        // e.printStackTrace();
                    }
                }
            }

            @Override
            public void connectionLost(Throwable cause) {
                Toast.makeText(NewTaskActivity.this, "Connection Lost", Toast.LENGTH_LONG).show();
                try {
                    client.connect(options);
                } catch (MqttException e) {
                    // e.printStackTrace();
                }

            }

            @Override
            public void messageArrived(String topic, MqttMessage message) {
                String receivedMsg = message.toString();
                if (receivedMsg.equals("ok")) {
                    Intent intent = new Intent(NewTaskActivity.this, MainActivity.class);
                    intent.putExtra("DevID", devID);
                    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                    startActivity(intent);
                }
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {

            }
        });
    }

    private String calculateTimeZones(String onTime, String offTime) {
        String[] dayOnTime = onTime.split(":");
        String getStartHour = dayOnTime[0];
        String startMin = dayOnTime[1];

        String[] dayOffTime = offTime.split(":");
        String getEndHour = dayOffTime[0];
        String endMin = dayOffTime[1];

        String startTime = String.format(new Locale("en"), "%02d:%02d", Integer.valueOf(getStartHour), Integer.valueOf(startMin));
        String endTime = String.format(new Locale("en"), "%02d:%02d", Integer.valueOf(getEndHour), Integer.valueOf(endMin));
        return startTime.concat(":").concat(endTime);
    }

    private boolean validateFields() {
        boolean valid = false;
        totalReps = (mSunday.concat(mMonday).concat(mTuesday).concat(mWednesday).concat(mThursday).concat(mFriday).concat(mSaturday));

        if (mSunday.equalsIgnoreCase("1")) {
            if (!sunOnTime.equalsIgnoreCase("Set On time") && !sunOnTime.isEmpty() && !sunOffTime.equalsIgnoreCase("Set Off time") && !sunOffTime.isEmpty()) {
                valid = true;
                sundayTime = calculateTimeZones(sunOnTime, sunOffTime);
                Log.d("Validation", "Sunday time calculated: " + sundayTime);
            } else {
                Log.d("Validation", "Invalid input for Sunday");
            }
        } else {
            sundayTime = "00:00:00:00";
        }
        if (mMonday.equalsIgnoreCase("1")) {
            if (!monOnTime.equalsIgnoreCase("Set On time") && !monOnTime.isEmpty() && !monOffTime.equalsIgnoreCase("Set Off time")) {
                valid = true;
                mondayTime = calculateTimeZones(monOnTime, monOffTime);
                Log.d("Validation", "Monday time calculated: " + mondayTime);
            } else {
                valid = false;
                Log.d("Validation", "Invalid input for Monday");
            }
        } else {
            mondayTime = "00:00:00:00";
        }
        if (mTuesday.equalsIgnoreCase("1")) {
            if (!tueOnTime.equalsIgnoreCase("Set On time") && !tueOnTime.isEmpty() && !tueOffTime.equalsIgnoreCase("Set Off time")) {
                valid = true;
                tuesdayTime = calculateTimeZones(tueOnTime, tueOffTime);
                Log.d("Validation", "Tuesday time calculated: " + tuesdayTime);
            } else {
                valid = false;
                Log.d("Validation", "Invalid input for Tuesday");
            }
        } else {
            tuesdayTime = "00:00:00:00";
        }
        if (mWednesday.equalsIgnoreCase("1")) {
            if (!wedOnTime.equalsIgnoreCase("Set On time") && !wedOnTime.isEmpty() && !wedOffTime.equalsIgnoreCase("Set Off time")) {
                valid = true;
                wednesdayTime = calculateTimeZones(wedOnTime, wedOffTime);
                Log.d("Validation", "Wednesday time calculated: " + wednesdayTime);
            } else {
                valid = false;
                Log.d("Validation", "Invalid input for Wednesday");
            }
        } else {
            wednesdayTime = "00:00:00:00";
        }
        if (mThursday.equalsIgnoreCase("1")) {
            if (!thurOnTime.equalsIgnoreCase("Set On time") && !thurOnTime.isEmpty() && !thurOffTime.equalsIgnoreCase("Set Off time") && !thurOffTime.isEmpty()) {
                valid = true;
                thursdayTime = calculateTimeZones(thurOnTime, thurOffTime);
                Log.d("Validation", "Thursday time calculated: " + thursdayTime);
            } else {
                valid = false;
                Log.d("Validation", "Invalid input for Thursday");
            }
        } else {
            thursdayTime = "00:00:00:00";
        }
        if (mFriday.equalsIgnoreCase("1")) {
            if (!friOnTime.equalsIgnoreCase("Set On time") && !friOnTime.isEmpty() && !friOffTime.equalsIgnoreCase("Set Off time")) {
                valid = true;
                fridayTime = calculateTimeZones(friOnTime, friOffTime);
                Log.d("Validation", "Friday time calculated: " + fridayTime);
            } else {
                valid = false;
                Log.d("Validation", "Invalid input for Friday");
            }
        } else {
            fridayTime = "00:00:00:00";
        }
        if (mSaturday.equalsIgnoreCase("1")) {
            if (!satOnTime.equalsIgnoreCase("Set On time") && !satOnTime.isEmpty() && !satOffTime.equalsIgnoreCase("Set Off time")) {
                valid = true;
                saturdayTime = calculateTimeZones(satOnTime, satOffTime);
                Log.d("Validation", "Saturday time calculated: " + saturdayTime);
            } else {
                valid = false;
                Log.d("Validation", "Invalid input for Saturday");
            }
        } else {
            saturdayTime = "00:00:00:00";
        }
        return valid;
    }

    @Override
    public void onItemSelected(AdapterView<?> adapterView, View view, int i, long l) {
        switch (i) {
            case 0:
                timeDifference = 8;
                break;
            case 1:
                timeDifference = 7;
                break;
            case 2:
                timeDifference = 6;
                break;
            case 3:
                timeDifference = 5;
                break;
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> adapterView) {

    }
}
