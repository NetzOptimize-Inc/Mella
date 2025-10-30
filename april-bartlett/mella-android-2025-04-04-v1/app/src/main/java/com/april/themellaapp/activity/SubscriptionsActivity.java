package com.april.themellaapp.activity;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.android.billingclient.api.AcknowledgePurchaseParams;
import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClientStateListener;
import com.android.billingclient.api.BillingFlowParams;
import com.android.billingclient.api.BillingResult;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.SkuDetails;
import com.android.billingclient.api.SkuDetailsParams;
import com.april.themellaapp.R;
import com.april.themellaapp.SubscriptionType;
import com.april.themellaapp.utils.Constants;

import java.util.Arrays;
import java.util.List;

@SuppressWarnings("deprecation")
public class SubscriptionsActivity extends AppCompatActivity implements View.OnClickListener {
    private TextView tvRestore;
    private BillingClient billingClient;

    private final PurchasesUpdatedListener purchasesUpdatedListener = (billingResult, purchases) -> {
        if ((billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK || billingResult.getResponseCode() == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) && purchases != null) {
            for (Purchase purchase : purchases) {
                Log.e("Purchased", String.valueOf(purchase.getPurchaseState()));
                for (String sku : purchase.getSkus()) {
                    Log.e("onPurchaseUpdate", sku);
                }

                if (!purchase.isAcknowledged()) {
                    AcknowledgePurchaseParams params = AcknowledgePurchaseParams.newBuilder()
                            .setPurchaseToken(purchase.getPurchaseToken())
                            .build();
                    billingClient.acknowledgePurchase(params, billingResult1 -> {
                        Log.e("AcknowledgeResponseListener", billingResult1.getDebugMessage());
                        Log.e("AcknowledgeResponseListener", billingResult1.toString());
                    });
                }
            }
            onPurchaseUpdate();
        } else if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.USER_CANCELED) {
            // Handle an error caused by a user cancelling the purchase flow.
            Log.e("onPurchasesUpdated", "Cancelled by User");
        } else {
            // Handle any other error codes.
            Log.e("onPurchasesUpdated", billingResult.getDebugMessage());
            Toast.makeText(this, "Something went wrong. Please try again later.", Toast.LENGTH_SHORT).show();
        }
    };

    private SkuDetails mSkuDetailsMonthly;
    private SkuDetails mSkuDetailsYearly;
    private SubscriptionType mSubscriptionType = SubscriptionType.NONE;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_subscriptions);

        initLayout();
        initBillingClient();
    }

    private void initLayout() {
        tvRestore = findViewById(R.id.tv_restore_subscription);
        tvRestore.setOnClickListener(this);
        tvRestore.setVisibility(View.GONE);

        findViewById(R.id.tv_annually_subscription).setOnClickListener(this);
        findViewById(R.id.tv_monthly_subscription).setOnClickListener(this);
        findViewById(R.id.tv_terms).setOnClickListener(this);
        findViewById(R.id.tv_privacy).setOnClickListener(this);

        // Version Number
        TextView tvVersion = findViewById(R.id.tv_version);
        try {
            PackageInfo pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            String version = pInfo.versionName;
            int versionCode = pInfo.versionCode;

            Log.e("RIDA", version + "(" + versionCode + ")");
            tvVersion.setText(String.format("Version: %s (%s)", version, versionCode));
        } catch (PackageManager.NameNotFoundException e) {
            // e.printStackTrace();
        }
    }

    private void initBillingClient() {
        billingClient = BillingClient.newBuilder(this)
                .setListener(purchasesUpdatedListener)
                .enablePendingPurchases()
                .build();

        billingClient.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(@NonNull BillingResult billingResult) {
                Log.e("startConnection", "onBillingSetupFinished");
                if (billingResult.getResponseCode() == BillingClient.BillingResponseCode.OK) {
                    // The BillingClient is ready. You can query purchases here.
                    Log.e("startConnection", "The BillingClient is ready");
                    showProductsAvailable();

                    billingClient.queryPurchasesAsync(BillingClient.SkuType.SUBS, (billingResult1, list) -> {
                        for (Purchase purchase : list) {
                            Log.e("queryPurchasesAsync", purchase.getSkus().stream().toString());
                            for (String sku :
                                    purchase.getSkus()) {
                                if (sku.equals(Constants.iab_monthly)) { // Visible
                                    mSubscriptionType = SubscriptionType.MONTHLY;
                                    tvRestore.setVisibility(View.VISIBLE);
                                } else if (sku.equals(Constants.iab_annually)) { // Visible
                                    mSubscriptionType = SubscriptionType.ANNUALLY;
                                    tvRestore.setVisibility(View.VISIBLE);
                                } else { // None
                                    tvRestore.setVisibility(View.GONE);
                                }
                            }
                        }
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

    private void showProductsAvailable() {
        List<String> skuList = Arrays.asList(
                Constants.iab_monthly,
                Constants.iab_annually
        );

        SkuDetailsParams.Builder params = SkuDetailsParams.newBuilder();
        params.setSkusList(skuList).setType(BillingClient.SkuType.SUBS);

        billingClient.querySkuDetailsAsync(params.build(), (billingResult, skuDetailsList) -> {
            if (skuDetailsList == null) {
                return;
            }

            // Process the result.
            for (SkuDetails skuDetails : skuDetailsList) {
                Log.e("querySkuDetailsAsync", skuDetails.getSku());
                if (skuDetails.getSku().equals(Constants.iab_monthly)) {
                    mSkuDetailsMonthly = skuDetails;
                } else if (skuDetails.getSku().equals(Constants.iab_annually)) {
                    mSkuDetailsYearly = skuDetails;
                }
            }
        });
    }

    private void onPurchaseUpdate() {
        Intent returnIntent = new Intent();
        returnIntent.putExtra("purchase", true);
        setResult(Activity.RESULT_OK, returnIntent);
        finish();
    }

    private void subscribe(SkuDetails skuDetails) {
        if (skuDetails == null) {
            Toast.makeText(this, "We can not find your IAB product available. Please try again later.", Toast.LENGTH_SHORT).show();
            return;
        }

        BillingFlowParams billingFlowParams = BillingFlowParams.newBuilder().setSkuDetails(skuDetails).build();
        int responseCode = billingClient.launchBillingFlow(this, billingFlowParams).getResponseCode();
        if (responseCode == BillingClient.BillingResponseCode.OK) {
            // Successfully Launched
            Log.e("launchBillingFlow", "Success");
        }
    }

    @Override
    public void onClick(View v) {
        if (v.getId() == R.id.tv_privacy) {
            Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://app.termly.io/document/privacy-policy/d2c6e19f-464b-480f-bdc1-bae891474737"));
            startActivity(browserIntent);
        } else if (v.getId() == R.id.tv_terms) {
            Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://app.termly.io/document/terms-of-use-for-saas/dffbbf12-9335-489b-b9ab-088fb66067a3"));
            startActivity(browserIntent);
        } else if (v.getId() == R.id.tv_restore_subscription) {
            if (mSubscriptionType == SubscriptionType.MONTHLY)
                subscribe(mSkuDetailsMonthly);
            else if (mSubscriptionType == SubscriptionType.ANNUALLY)
                subscribe(mSkuDetailsYearly);
        } else if (v.getId() == R.id.tv_annually_subscription) {
            subscribe(mSkuDetailsYearly);
        } else if (v.getId() == R.id.tv_monthly_subscription) {
            subscribe(mSkuDetailsMonthly);
        }
    }
}
