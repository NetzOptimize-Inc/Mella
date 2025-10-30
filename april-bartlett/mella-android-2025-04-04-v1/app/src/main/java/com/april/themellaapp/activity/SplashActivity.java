package com.april.themellaapp.activity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.april.themellaapp.R;
import com.april.themellaapp.utils.Constants;
import com.google.firebase.auth.FirebaseAuth;

public class SplashActivity extends AppCompatActivity {
    FirebaseAuth firebaseAuth;
    private final ActivityResultLauncher<Intent> subscriptionLauncher = registerForActivityResult(
            new ActivityResultContracts.StartActivityForResult(),
            result -> {
                Intent data = result.getData();
                if (data == null) {
                    return;
                }
                loadNextScreen();
            });
    private BillingClient billingClient;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_splash);

        // Get Firebase auth instance
        firebaseAuth = FirebaseAuth.getInstance();
        initBillingClient();
    }

    private void loadNextScreen() {
        getDetails();
    }

    private void getDetails() {
        SharedPreferences sharedPreferences = getApplicationContext().getSharedPreferences("UserDetails", MODE_PRIVATE);
        if ((!sharedPreferences.getString("UserEmail", "").isEmpty()) && (!sharedPreferences.getString("UserPass", "").isEmpty())) {
            loginFirebase(sharedPreferences.getString("UserEmail", ""), sharedPreferences.getString("UserPass", ""));
        } else {
            Intent intent = new Intent(SplashActivity.this, LoginActivity.class);
            startActivity(intent);
            finish();
        }
    }

    private void loginFirebase(final String mail, final String pass) {
        // Authenticate User
        firebaseAuth.signInWithEmailAndPassword(mail, pass).addOnCompleteListener(SplashActivity.this, task -> {
            // If sign in fails, display a message to the user. If sign in succeeds
            // the auth state listener will be notified and logic to handle the
            // signed in user can be handled in the listener.
            if (!task.isSuccessful()) {
                SharedPreferences sharedPreferences = getApplicationContext().getSharedPreferences("UserDetails", MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPreferences.edit();
                editor.remove("UserEmail");
                editor.apply();

                // there was an error
                loadNextScreen();
            } else {
                SharedPreferences sharedPreferences = getApplicationContext().getSharedPreferences("UserDetails", MODE_PRIVATE);
                SharedPreferences.Editor editor = sharedPreferences.edit();
                editor.putString("UserEmail", mail);
                editor.putString("UserPass", pass);
                editor.apply();

                Intent intent = new Intent(SplashActivity.this, DeviceListActivity.class);
                startActivity(intent);
                finish();
            }
        });
    }

    private void presentSubscription() {
        Intent intent = new Intent(this, SubscriptionsActivity.class);
        subscriptionLauncher.launch(intent);
    }

    private void initBillingClient() {
        billingClient = BillingClient.newBuilder(this)
                .setListener((billingResult, list) -> {
                    // nothing
                })
                .enablePendingPurchases()
                .build();

        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            @SuppressWarnings("deprecation")
            public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                Log.e("startConnection", "onBillingSetupFinished");
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    // The BillingClient is ready. You can query purchases here.
                    Log.e("startConnection", "The BillingClient is ready");

                    billingClient.queryPurchasesAsync(BillingClient.SkuType.SUBS, (billingResult1, list) -> {
                        Log.e("queryPurchasesAsync -- ", "Purchased Count: " + list.size());
                        for (Purchase purchase : list) {
                            for (String sku :
                                    purchase.getSkus()) {
                                if (sku.equals(Constants.iab_monthly) || sku.equals(Constants.iab_annually)) { // Visible
                                    loadNextScreen();
                                    return;
                                }
                            }
                        }
                        presentSubscription();
                    });
                }
            }

            @Override
            public void onBillingServiceDisconnected() {
                // Try to restart the connection on the next request to
                // Google Play by calling the startConnection() method.
                Log.e("startConnection", "onBillingServiceDisconnected");
            }
        });
    }
}
