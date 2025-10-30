package com.april.themellaapp.activity;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;

import com.april.themellaapp.R;

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
import org.jetbrains.annotations.NotNull;

import java.io.IOException;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.Response;
import okhttp3.ResponseBody;

public class Update extends Activity {
    TextView checkUpdate;
    TextView updateIV;
    TextView updateStatus;
    ImageView navBack;
    ProgressBar progressBar;
    MyCountDownTimer myCountDownTimer;

    MqttAndroidClient client;
    MemoryPersistence persistence = new MemoryPersistence();
    MqttConnectOptions options = new MqttConnectOptions();

    private OkHttpClient okHttpClient;
    private Request request;
    private String url = "http://204.48.18.117/OTA/index.php?version=";

    String res;
    String devID;


    @SuppressLint("SetTextI18n")
    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.update_window);
        navBack = findViewById(R.id.navBack_Update);

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);

        int width = dm.widthPixels;
        int height = dm.heightPixels;
        getWindow().setLayout((width), (height));

        checkUpdate = findViewById(R.id.check_update_text_view);
        updateIV = findViewById(R.id.update_text_view);
        updateStatus = findViewById(R.id.update_tv);
        progressBar = findViewById(R.id.progressbar);

        okHttpClient = new OkHttpClient();

        String clientId = MqttClient.generateClientId();
        client = new MqttAndroidClient(this.getApplicationContext(), "tcp://204.48.18.117:1883", clientId, persistence);

        Bundle bundle = getIntent().getExtras();
        if (bundle != null) {
            devID = bundle.getString("DevID");
        }

        SharedPreferences sharedPreferences = getApplicationContext().getSharedPreferences("UserDetails", MODE_PRIVATE);
        if ((!sharedPreferences.getString("devID", "").isEmpty()) && (!sharedPreferences.getString("version", "").isEmpty())) {
            String version = sharedPreferences.getString("version", "");
            url = url + version;
        } else {
            url = url + "50";
        }

        checkUpdate.setOnClickListener(v -> {
            request = new Request.Builder()
                    .url(url)
                    .build();

            okHttpClient.newCall(request).enqueue(new Callback() {
                @Override
                public void onFailure(@NotNull Call call, @NotNull IOException e) {
                    // e.printStackTrace();
                }

                @SuppressLint("SetTextI18n")
                @Override
                public void onResponse(@NotNull Call call, @NotNull Response response) throws IOException {
                    if (response.isSuccessful()) {
                        ResponseBody responseBody = response.body();
                        if (responseBody == null)
                            return;
                        res = responseBody.string();
                        if (res.equals("No Update")) {
                            Update.this.runOnUiThread(() -> updateStatus.setText("No Update Available...."));
                        } else {
                            Update.this.runOnUiThread(() -> {
                                checkUpdate.setVisibility(View.GONE);
                                updateIV.setVisibility(View.VISIBLE);
                                updateStatus.setText("Version " + res + "is available.");
                                initMqtt();
                            });
                        }
                    }
                }
            });
        });

        updateIV.setOnClickListener(v -> {
            updateIV.setClickable(false);
            updateStatus.setText("Please wait while firmware is upgrading. Green Light turn on after successful upgrade on device...");
            String updateData = "1";

            try {
                String publishId = devID + "/Upgrade";
                if (client.isConnected()) {
                    client.publish(publishId, updateData.getBytes(), 1, true);
                    client.acknowledgeMessage("ACK");
                }
            } catch (MqttException e) {
                // e.printStackTrace();
            }
            progressBar.setVisibility(View.VISIBLE);
            progressBar.setProgress(100);
            myCountDownTimer = new MyCountDownTimer(20000, 500);
            myCountDownTimer.start();
        });

        navBack.setOnClickListener(v -> startActivity(new Intent(Update.this, MainActivity.class)));
    }

    public void initMqtt() {
        try {
            options.setUserName("themella");
            options.setPassword("!@#mqtt".toCharArray());
            options.setAutomaticReconnect(true);
            options.setMqttVersion(MqttConnectOptions.MQTT_VERSION_3_1);
            IMqttToken token = client.connect(options);
            token.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    // empty
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Toast.makeText(Update.this, "Connection Failed", Toast.LENGTH_LONG).show();
                }
            });
        } catch (MqttException e) {
            // e.printStackTrace();
        }

        client.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectionLost(Throwable cause) {
                Toast.makeText(Update.this, "Connection Lost", Toast.LENGTH_LONG).show();
                try {
                    client.connect(options);
                } catch (MqttException e) {
                    // e.printStackTrace();
                }
            }

            @Override
            public void messageArrived(String topic, MqttMessage message) {
                // empty
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
                // empty
            }

            @Override
            public void connectComplete(boolean reconnect, String s) {
                // empty
            }
        });
    }

    public class MyCountDownTimer extends CountDownTimer {
        public MyCountDownTimer(long millisInFuture, long countDownInterval) {
            super(millisInFuture, countDownInterval);
        }

        @Override
        public void onTick(long millisUntilFinished) {
            int progress = (int) (millisUntilFinished / 100);
            progressBar.setProgress(progress);
        }

        @Override
        public void onFinish() {
            progressBar.setProgress(0);
            startActivity(new Intent(Update.this, MainActivity.class));
            finish();
        }
    }
}
