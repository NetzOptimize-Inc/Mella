package com.april.themellaapp.activity;

import android.Manifest;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.april.themellaapp.EspWifiAdminSimple;
import com.april.themellaapp.R;
import com.espressif.iot.esptouch.EsptouchTask;
import com.espressif.iot.esptouch.IEsptouchListener;
import com.espressif.iot.esptouch.IEsptouchResult;
import com.espressif.iot.esptouch.IEsptouchTask;
import com.espressif.iot.esptouch.task.__IEsptouchTask;

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

import java.lang.ref.WeakReference;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "MainActivity";
    private static final int REQUEST_ACCESS_FINE_LOCATION = 111;

    static String MQTT_HOST = "tcp://167.71.112.166:1883"; // "tcp://204.48.18.117:1883"; //
    static String USERNAME = "themella";
    static String PASSWORD = "!@#mqtt";

    private final IEsptouchListener myListener = this::onEspTouchResultAddedPerform;

    TextView mScheduleTextView, statusTextView;
    ImageView controlButton;
    ImageView updateView;
    ImageView navigateBackMain;

    String subscribeStatus = "/Status";
    String subscribeUpdate = "/Publish";
    String publishAction = "/Action";
    String publishSchedule = "/Schedule";
    String publishAutoShutdown = "/Autoshutdown";
    String payload;
    String devID;

    boolean actionStatus = false;

    MqttAndroidClient client;
    MemoryPersistence persistence = new MemoryPersistence();
    MqttConnectOptions options = new MqttConnectOptions();

    private EspWifiAdminSimple mWifiAdmin;
    private EsptouchAsyncTask mTask;

    public MainActivity() {
        // empty
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mWifiAdmin = new EspWifiAdminSimple(this);
        controlButton = findViewById(R.id.mellButton);
        statusTextView = findViewById(R.id.status_text_view);
        navigateBackMain = findViewById(R.id.navigate_back_main);

        String clientId = MqttClient.generateClientId();
        client = new MqttAndroidClient(this.getApplicationContext(), MQTT_HOST, clientId, persistence);

        initMqtt();
        initXMLComponents();

        Bundle bundle = getIntent().getExtras();
        if (bundle != null) {
            devID = bundle.getString("DevID");
            Log.d("BundleTAG", "Device ID retrieved: " + devID);

            subscribeStatus = devID + "/Status";
            Log.d("BundleTAG", "Subscribe Topic: " + subscribeStatus);

            subscribeUpdate = devID + "/Update";
            Log.d("BundleTAG", "Subscribe Topic Update: " + subscribeUpdate);

            publishAction = devID + "/Action";
            Log.d("BundleTAG", "Publish Topic 1: " + publishAction);

            publishSchedule = devID + "/Schedule";
            Log.d("BundleTAG", "Publish Topic 2: " + publishSchedule);

            publishAutoShutdown = devID + "/Autoshutdown";
            Log.d("BundleTAG", "Publish Topic 3: " + publishAutoShutdown);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQUEST_ACCESS_FINE_LOCATION) {
            if (permissions.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                checkFineLocationPermission();
            }
        }
    }

    public void initMqtt() {
        Log.d("InitMqqt", "initMqtt");
        try {
            options.setUserName(USERNAME);
            options.setPassword(PASSWORD.toCharArray());
            options.setAutomaticReconnect(true);
            options.setMqttVersion(MqttConnectOptions.MQTT_VERSION_3_1);

            IMqttToken token = client.connect(options);
            token.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    Log.e("InitMqqt", "onSuccess: ");
                    subscribe();
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Log.e("InitMqqt", "Connection failed: " + exception.getMessage());
                    Toast.makeText(MainActivity.this, "Connection Failed", Toast.LENGTH_LONG).show();
                }
            });
        } catch (Exception e) {
            Log.d("InitMqqt", "MqttException: " + e.getMessage());
            // e.printStackTrace();
            Toast.makeText(MainActivity.this, "MqttException: " + e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
        }

        client.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectionLost(Throwable cause) {
                Log.d("InitMqqt", "Connection lost: " + cause.getMessage());
                Toast.makeText(MainActivity.this, "Connection Lost: " + cause.getLocalizedMessage(), Toast.LENGTH_LONG).show();

                controlButton.setImageResource(R.drawable.disable_button);
                try {
                    client.connect(options);
                } catch (MqttException e) {
                    Log.d("InitMqqt", "Failed to reconnect: " + e.getMessage());
                    // e.printStackTrace();
                }
            }

            @Override
            public void messageArrived(String topic, MqttMessage message) {
                Log.d("InitMqqt", "Message received on topic: " + topic + ", message: " + message.toString());

                if (message.toString().equals("1")) {
                    controlButton.setImageResource(R.drawable.on_button);
                    statusTextView.setText(R.string.status_on);
                }
                if (message.toString().equals("0")) {
                    controlButton.setImageResource(R.drawable.off_button);
                    statusTextView.setText(R.string.status_off);
                }
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
                Log.d("InitMqqt", "deliveryComplete");
            }

            @Override
            public void connectComplete(boolean reconnect, String s) {
                Log.d("InitMqqt", "connectComplete");
                if (reconnect) {
                    subscribe();
                }
            }
        });
    }

    private void initXMLComponents() {
        mScheduleTextView = findViewById(R.id.schedule_text_view);
        navigateBackMain.setOnClickListener(v -> startActivity(new Intent(MainActivity.this, DeviceListActivity.class)));

        updateView = findViewById(R.id.update_btn);
        updateView.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, Update.class);
            intent.putExtra("DevID", devID);
            startActivity(intent);
        });

        controlButton.setImageResource(R.drawable.disable_button);
        controlButton.setOnClickListener(v -> showStatus());

        mScheduleTextView.setOnClickListener(view -> {
            if (client.isConnected()) {
                Log.d("MainActivity", "Client is connected, starting NewTaskActivity");
                Intent intent = new Intent(MainActivity.this, NewTaskActivity.class);
                intent.putExtra("DevID", devID);
                startActivity(intent);
            } else {
                Log.d("MainActivity", "Client is not connected");
                // Handle the case when the client is not connected
            }
        });

        statusTextView.setOnClickListener(v -> checkFineLocationPermission());
    }

    private void showStatus() {
        if (!actionStatus) {
            payload = "1";
            actionStatus = true;
        } else {
            payload = "0";
            actionStatus = false;
        }

        Log.d("controlButtonTAG", "Payload: " + payload); // Log payload value
        try {
            if (client.isConnected()) {
                Log.d("controlButtonTAG", "Publishing message...");
                client.publish(publishAction, payload.getBytes(), 1, true);
                client.acknowledgeMessage("ACK");
                Log.d("controlButtonTAG", "Message published successfully");
            }
        } catch (MqttException e) {
            // e.printStackTrace();
            Log.e("controlButtonTAG", "Error publishing message: " + e.getMessage()); // Log error
            Toast.makeText(this, e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    private void checkFineLocationPermission() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED
                || ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_NETWORK_STATE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_NETWORK_STATE}, REQUEST_ACCESS_FINE_LOCATION);
        } else {
            onClickStatus();
        }
    }

    @SuppressWarnings("deprecation")
    private void onClickStatus() {
        String testString = statusTextView.getText().toString();
        if (testString.equals("NOT CONNECTED")) {
            AlertDialog.Builder mBuilder = new AlertDialog.Builder(MainActivity.this);
            View mView = getLayoutInflater().inflate(R.layout.config_wifi, null);

            final EditText ssidText = mView.findViewById(R.id.ssidEditText);
            final EditText passText = mView.findViewById(R.id.passEditText);

            TextView confirmButton = mView.findViewById(R.id.confirm_text_view);
            confirmButton.setOnClickListener(v1 -> {
                if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_NETWORK_STATE) != PackageManager.PERMISSION_GRANTED) {
                    return;
                }

                if (!ssidText.getText().toString().isEmpty() && !passText.getText().toString().isEmpty()) {
                    ConnectivityManager connManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
                    NetworkInfo activeNetwork = connManager.getActiveNetworkInfo();
                    if (activeNetwork != null && activeNetwork.getType() == ConnectivityManager.TYPE_WIFI && activeNetwork.isConnected()) {
                        // Do whatever
                        String apSsid = ssidText.getText().toString();
                        String apPassword = passText.getText().toString();
                        String apBssid = mWifiAdmin.getWifiConnectedBssid();

                        String taskResultCountStr = "1";
                        if (__IEsptouchTask.DEBUG) {
                            Log.d(TAG, "mBtnConfirm is clicked, mEdtApSsid = " + apSsid + ", " + " mEdtApPassword = " + apPassword);
                        }
                        if (mTask != null) {
                            mTask.cancelEspTouch();
                        }
                        mTask = new EsptouchAsyncTask(MainActivity.this);
                        mTask.execute(apSsid, apBssid, apPassword, taskResultCountStr);
                    } else {
                        Toast.makeText(MainActivity.this, "Please connect to wifi...", Toast.LENGTH_LONG).show();
                    }
                }
            });

            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_NETWORK_STATE) == PackageManager.PERMISSION_GRANTED) {
                ConnectivityManager connManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
                NetworkInfo activeNetwork = connManager.getActiveNetworkInfo();
                if (activeNetwork != null && activeNetwork.getType() == ConnectivityManager.TYPE_WIFI && activeNetwork.isConnected()) {
                    // Do whatever
                    ssidText.setText(mWifiAdmin.getWifiConnectedSsid());
                } else {
                    Toast.makeText(MainActivity.this, "Please connect to wifi...", Toast.LENGTH_LONG).show();
                }
            }

            mBuilder.setView(mView);
            AlertDialog dialog = mBuilder.create();
            dialog.show();
        }
    }

    private void onEspTouchResultAddedPerform(final IEsptouchResult result) {
        runOnUiThread(() -> {
            String text = result.getBssid() + " is connected to the wifi";
            Toast.makeText(MainActivity.this, text, Toast.LENGTH_LONG).show();
        });
    }

    public void subscribe() {
        try {
            client.subscribe(subscribeStatus, 0);
            client.subscribe(subscribeUpdate, 0);
        } catch (MqttException e) {
            // e.printStackTrace();
        }
    }

    private static class EsptouchAsyncTask extends AsyncTask<String, Void, List<IEsptouchResult>> {
        private final WeakReference<MainActivity> mActivity;

        // without the lock, if the user tap confirm and cancel quickly enough,
        // the bug will arise. the reason is follows:
        // 0. task is starting created, but not finished
        // 1. the task is cancel for the task hasn't been created, it do nothing
        // 2. task is created
        // 3. Oops, the task should be cancelled, but it is running
        private final Object mLock = new Object();
        private ProgressDialog mProgressDialog;
        private IEsptouchTask mEspTouchTask;

        EsptouchAsyncTask(MainActivity activity) {
            mActivity = new WeakReference<>(activity);
        }

        @SuppressWarnings("deprecation")
        public void cancelEspTouch() {
            cancel(true);
            if (mProgressDialog != null) {
                mProgressDialog.dismiss();
            }
            if (mEspTouchTask != null) {
                mEspTouchTask.interrupt();
            }
        }

        @Override
        @SuppressWarnings("deprecation")
        protected void onPreExecute() {
            Activity activity = mActivity.get();
            mProgressDialog = new ProgressDialog(activity);
            mProgressDialog.setMessage("EspTouch is configuring, please wait for a moment...");
            mProgressDialog.setCanceledOnTouchOutside(false);
            mProgressDialog.setOnCancelListener(dialog -> {
                synchronized (mLock) {
                    if (__IEsptouchTask.DEBUG) {
                        Log.i(TAG, "progress dialog back pressed canceled");
                    }
                    if (mEspTouchTask != null) {
                        mEspTouchTask.interrupt();
                    }
                }
            });
            mProgressDialog.setButton(DialogInterface.BUTTON_NEGATIVE, activity.getText(android.R.string.cancel),
                    (dialog, which) -> {
                        synchronized (mLock) {
                            if (__IEsptouchTask.DEBUG) {
                                Log.i(TAG, "progress dialog cancel button canceled");
                            }
                            if (mEspTouchTask != null) {
                                mEspTouchTask.interrupt();
                            }
                        }
                    });
            mProgressDialog.show();
        }

        @Override
        @SuppressWarnings("deprecation")
        protected List<IEsptouchResult> doInBackground(String... params) {
            MainActivity activity = mActivity.get();
            int taskResultCount;
            synchronized (mLock) {
                String apSsid = params[0];
                String apBssid = params[1];
                String apPassword = params[2];
                String taskResultCountStr = params[3];
                taskResultCount = Integer.parseInt(taskResultCountStr);
                Context context = activity.getApplicationContext();

                mEspTouchTask = new EsptouchTask(apSsid, apBssid, apPassword, null, context);
                mEspTouchTask.setEsptouchListener(activity.myListener);
            }
            return mEspTouchTask.executeForResults(taskResultCount);
        }

        @Override
        @SuppressWarnings("deprecation")
        protected void onPostExecute(List<IEsptouchResult> result) {
            Activity activity = mActivity.get();
            if (mProgressDialog.isShowing())
                mProgressDialog.dismiss();
            AlertDialog mResultDialog = new AlertDialog.Builder(activity)
                    .setPositiveButton(android.R.string.ok, null)
                    .create();
            mResultDialog.setCanceledOnTouchOutside(false);
            if (result == null) {
                mResultDialog.setMessage("Create EspTouch task failed, the port could be used by other thread");
                mResultDialog.show();
                return;
            }

            // check whether the task is cancelled and no results received
            IEsptouchResult firstResult = result.get(0);
            if (!firstResult.isCancelled()) {
                int count = 0;
                // max results to be displayed, if it is more than maxDisplayCount,
                // just show the count of redundant ones
                final int maxDisplayCount = 5;
                // the task received some results including cancelled while
                // executing before receiving enough results
                if (firstResult.isSuc()) {
                    StringBuilder sb = new StringBuilder();
                    for (IEsptouchResult resultInList : result) {
                        sb.append("Esptouch success\nbssid = ")
                                .append(resultInList.getBssid())
                                .append("\nInetAddress = ")
                                .append(resultInList.getInetAddress().getHostAddress())
                                .append("\n");
                        count++;
                        if (count >= maxDisplayCount) {
                            break;
                        }
                    }
                    if (count < result.size()) {
                        sb.append("\nthere's ")
                                .append(result.size() - count)
                                .append(" more result(s) without showing\n");
                    }
                    mResultDialog.setMessage(sb.toString());
                } else {
                    mResultDialog.setMessage("Esptouch fail");
                }
                mResultDialog.show();
            }
        }
    }
}
