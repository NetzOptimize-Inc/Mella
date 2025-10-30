package com.april.themellaapp.activity;

import android.Manifest;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.april.themellaapp.EspWifiAdminSimple;
import com.april.themellaapp.R;
import com.april.themellaapp.utils.FirebaseDatabaseReference;
import com.blikoon.qrcodescanner.QrCodeActivity;
import com.espressif.iot.esptouch.EsptouchTask;
import com.espressif.iot.esptouch.IEsptouchListener;
import com.espressif.iot.esptouch.IEsptouchResult;
import com.espressif.iot.esptouch.IEsptouchTask;
import com.espressif.iot.esptouch.task.__IEsptouchTask;
import com.google.firebase.database.DatabaseReference;

import java.lang.ref.WeakReference;
import java.util.List;

public class NewDeviceActivity extends AppCompatActivity {
    private static final String TAG = "NewTaskActivity";
    private static final int REQUEST_ACCESS_FINE_LOCATION = 111;
    private static final String LOG_TAG = "ScanQRCode";
    private static final int REQUEST_CODE_QR_SCAN = 101;
    static Context context;
    private final IEsptouchListener myListener = this::onEspTouchResultAddedPerform;
    TextView mScanQRTextView, mSetupDeviceTextView;
    ImageView mNavigateBackImageView;
    EditText devIdText, nameEditText;
    TextView logoutView;
    SharedPreferences sharedPreferences;
    SharedPreferences.Editor editor;
    ImageView imageView;
    int check = 0;
    String devName;
    String devID;
    DatabaseReference databaseReference;
    private EspWifiAdminSimple mWifiAdmin;
    private EspTouchAsyncTask3 mTask;
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        @SuppressWarnings("deprecation")
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action == null) {
                return;
            }

            if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
                NetworkInfo ni = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
                if (ni != null && !ni.isConnected()) {
                    if (mTask != null) {
                        mTask.cancelEspTouch();
                        mTask = null;
                        new AlertDialog.Builder(NewDeviceActivity.this)
                                .setMessage("Wifi disconnected or changed")
                                .setNegativeButton(android.R.string.cancel, null)
                                .show();
                    }
                }
            }
        }
    };

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_new_device);

        context = this;
        mWifiAdmin = new EspWifiAdminSimple(this);

        sharedPreferences = getApplicationContext().getSharedPreferences("UserDetails", MODE_PRIVATE);
        editor = sharedPreferences.edit();

        Bundle bundle = getIntent().getExtras();
        if (bundle != null) {
            devName = bundle.getString("DevName");
            devID = bundle.getString("DevID");
            check = 1;
        }
        initXMLComponents();
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

    @SuppressWarnings("deprecation")
    private void initXMLComponents() {
        mNavigateBackImageView = findViewById(R.id.navigate_back);
        mNavigateBackImageView.setOnClickListener(view -> finish());

        logoutView = findViewById(R.id.logout_button);

        devIdText = findViewById(R.id.device_id_edit_text);
        nameEditText = findViewById(R.id.device_name_edit_text);
        imageView = findViewById(R.id.save_devid);

        mScanQRTextView = findViewById(R.id.scan_qr);

        if (check == 1) {
            if (!devName.isEmpty()) {
                nameEditText.setText(devName);
                devIdText.setText(devID);
                check = 0;
            }
        }

        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, 1);
        }

        imageView.setOnClickListener(v -> {
            if (!devIdText.getText().toString().isEmpty() && !nameEditText.getText().toString().isEmpty()) {
                saveDataToFirebase();
            } else {
                Toast.makeText(NewDeviceActivity.this, "Device ID or Name Cannot be empty..", Toast.LENGTH_SHORT).show();
            }
        });

        mScanQRTextView.setOnClickListener(view -> {
            Intent i = new Intent(NewDeviceActivity.this, QrCodeActivity.class);
            startActivityForResult(i, REQUEST_CODE_QR_SCAN);
        });

        mSetupDeviceTextView = findViewById(R.id.setup_device);
        mSetupDeviceTextView.setOnClickListener(v -> checkFineLocationPermission());

        findViewById(R.id.setup_skip).setOnClickListener(view -> {
            if (!devIdText.getText().toString().isEmpty() && !nameEditText.getText().toString().isEmpty()) {
                saveDataToFirebase();
                Toast.makeText(this, "Mella Setup Successful.", Toast.LENGTH_SHORT).show();

                finish();
            } else {
                Toast.makeText(NewDeviceActivity.this, "Device ID or Name Cannot be empty..", Toast.LENGTH_SHORT).show();
            }
        });

        logoutView.setOnClickListener(v -> {
            editor.putString("UserEmail", "");
            editor.putString("UserPass", "");
            editor.apply();

            Intent intent = new Intent(NewDeviceActivity.this, LoginActivity.class);
            startActivity(intent);
            finish();
        });
    }

    private void checkFineLocationPermission() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.ACCESS_FINE_LOCATION}, REQUEST_ACCESS_FINE_LOCATION);
        } else {
            presentSetupWifi();
        }
    }

    @SuppressWarnings("deprecation")
    private void presentSetupWifi() {
        AlertDialog.Builder mBuilder = new AlertDialog.Builder(NewDeviceActivity.this);
        View mView = getLayoutInflater().inflate(R.layout.config_wifi, null);
        final EditText ssidText = mView.findViewById(R.id.ssidEditText);
        final EditText passText = mView.findViewById(R.id.passEditText);
        TextView confirmButton = mView.findViewById(R.id.confirm_text_view);

        confirmButton.setOnClickListener(v1 -> {
            if (!ssidText.getText().toString().isEmpty() && !passText.getText().toString().isEmpty()) {
                ConnectivityManager connManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
                NetworkInfo activeNetwork = connManager.getActiveNetworkInfo();
                if (activeNetwork != null && activeNetwork.getType() == ConnectivityManager.TYPE_WIFI && activeNetwork.isConnected()) {
                    // Do whatever
                    String apSsid = ssidText.getText().toString();
                    String apPassword = passText.getText().toString();
                    String apBssid = mWifiAdmin.getWifiConnectedBssid();


                    if (!(devIdText.getText().toString().isEmpty())) {
                        saveDataToFirebase();
                    } else {
                        Toast.makeText(NewDeviceActivity.this, "Device ID Cannot be empty...", Toast.LENGTH_LONG).show();
                    }

                    String taskResultCountStr = "1";
                    if (__IEsptouchTask.DEBUG) {
                        Log.d(TAG, "mBtnConfirm is clicked, mEdtApSsid = " + apSsid + ", " + " mEdtApPassword = " + apPassword);
                    }
                    if (mTask != null) {
                        mTask.cancelEspTouch();
                    }
                    mTask = new EspTouchAsyncTask3(NewDeviceActivity.this);
                    mTask.execute(apSsid, apBssid, apPassword, taskResultCountStr);
                } else {
                    Toast.makeText(NewDeviceActivity.this, "Please connect to wifi...", Toast.LENGTH_LONG).show();
                }
            }
        });

        ConnectivityManager connManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetwork = connManager.getActiveNetworkInfo();
        if (activeNetwork != null && activeNetwork.getType() == ConnectivityManager.TYPE_WIFI && activeNetwork.isConnected()) {
            // Do whatever
            ssidText.setText(mWifiAdmin.getWifiConnectedSsid());
        } else {
            Toast.makeText(NewDeviceActivity.this, "Please connect to wifi...", Toast.LENGTH_LONG).show();
        }

        mBuilder.setView(mView);
        AlertDialog dialog = mBuilder.create();
        dialog.show();
    }

    private void saveDataToFirebase() {
        databaseReference = FirebaseDatabaseReference.deviceRef();
        String name = nameEditText.getText().toString().trim();
        String devID = devIdText.getText().toString().trim();

        try {
            databaseReference.child(name).setValue(devID);
            Toast.makeText(NewDeviceActivity.this, "Device Added Successfully...", Toast.LENGTH_LONG).show();
        } catch (Exception ex) {
            Toast.makeText(NewDeviceActivity.this, "Get Error while adding device...", Toast.LENGTH_LONG).show();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != Activity.RESULT_OK) {
            Log.d(LOG_TAG, "COULD NOT GET A GOOD RESULT.");
            if (data == null) {
                return;
            }

            // Getting the passed result
            String result = data.getStringExtra("com.blikoon.qrcodescanner.error_decoding_image");
            if (result != null) {
                AlertDialog alertDialog = new AlertDialog.Builder(NewDeviceActivity.this).create();
                alertDialog.setTitle("Scan Error");
                alertDialog.setMessage("QR Code could not be scanned");
                alertDialog.setButton(AlertDialog.BUTTON_NEUTRAL, "OK", (dialog, which) -> dialog.dismiss());
                alertDialog.show();
            }
            return;

        }
        if (requestCode == REQUEST_CODE_QR_SCAN) {
            if (data == null) {
                return;
            }

            // Getting the passed result
            String result = data.getStringExtra("com.blikoon.qrcodescanner.got_qr_scan_relult");
            Log.d(LOG_TAG, "Have scan result in your app activity :" + result);
            devIdText.setText(result);
        }
    }

    private void onEspTouchResultAddedPerform(final IEsptouchResult result) {
        runOnUiThread(() -> {
            String text = result.getBssid() + " is connected to the wifi";
            Toast.makeText(NewDeviceActivity.this, text, Toast.LENGTH_LONG).show();
        });
    }

    private static class EspTouchAsyncTask3 extends AsyncTask<String, Void, List<IEsptouchResult>> {
        private final WeakReference<NewDeviceActivity> mActivity;

        // without the lock, if the user tap confirm and cancel quickly enough,
        // the bug will arise. the reason is follows:
        // 0. task is starting created, but not finished
        // 1. the task is cancel for the task hasn't been created, it do nothing
        // 2. task is created
        // 3. Oops, the task should be cancelled, but it is running
        private final Object mLock = new Object();
        private ProgressDialog mProgressDialog;
        private IEsptouchTask mEspTouchTask;

        EspTouchAsyncTask3(NewDeviceActivity activity) {
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
            mProgressDialog.setMessage("Mella is configuring, please wait for a moment...");
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
            NewDeviceActivity activity = mActivity.get();
            int taskResultCount;
            synchronized (mLock) {
                // !!!NOTICE
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
            Log.d("result", String.valueOf(result));
            Activity activity = mActivity.get();
            mProgressDialog.dismiss();
            AlertDialog mResultDialog = new AlertDialog.Builder(activity).setPositiveButton(android.R.string.ok, null).create();
            mResultDialog.setCanceledOnTouchOutside(false);
            if (result == null) {
                mResultDialog.setMessage("Create EspTouch task failed, the port could be used by other thread");
                mResultDialog.show();
                return;
            }

            IEsptouchResult firstResult = result.get(0);
            // check whether the task is cancelled and no results received
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
                        sb.append("EspTouch success\nbssid = ")
                                .append(resultInList.getBssid())
                                .append("\nInetAddress = ")
                                .append(resultInList.getInetAddress().getHostAddress())
                                .append("\n");
                        count++;
                        Intent i = new Intent(activity, DeviceListActivity.class);
                        activity.startActivity(i);
                        activity.finish();
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
                    mResultDialog.setMessage("EspTouch fail");
                }

                mResultDialog.show();
            }
        }
    }
}
