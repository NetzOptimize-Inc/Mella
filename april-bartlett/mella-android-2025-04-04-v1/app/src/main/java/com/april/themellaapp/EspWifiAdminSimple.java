package com.april.themellaapp;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;

public class EspWifiAdminSimple {
    private final Context mContext;

    public EspWifiAdminSimple(Context context) {
        mContext = context;
    }

    public String getWifiConnectedSsid() {
        WifiInfo mWifiInfo = getConnectionInfo();
        String ssid = null;
        if (mWifiInfo != null && isWifiConnected()) {
            if (mWifiInfo.getSupplicantState() == SupplicantState.COMPLETED) {
                int len = mWifiInfo.getSSID().length();
                if (mWifiInfo.getSSID().startsWith("\"")
                        && mWifiInfo.getSSID().endsWith("\"")) {
                    ssid = mWifiInfo.getSSID().substring(1, len - 1);
                } else {
                    ssid = mWifiInfo.getSSID();
                }
            } else {
                ssid = "";
            }
        }
        return ssid;
    }

    public String getWifiConnectedBssid() {
        WifiInfo mWifiInfo = getConnectionInfo();
        String bssid = null;
        if (mWifiInfo != null && isWifiConnected()) {
            bssid = mWifiInfo.getBSSID();
        }
        return bssid;
    }

    private WifiInfo getConnectionInfo() {
        WifiManager wifiManager = (WifiManager) mContext.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        return wifiManager.getConnectionInfo();
    }

    private boolean isWifiConnected() {
        NetworkInfo mWiFiNetworkInfo = getWifiNetworkInfo();
        boolean isWifiConnected = false;
        if (mWiFiNetworkInfo != null) {
            isWifiConnected = mWiFiNetworkInfo.isConnected();
        }
        return isWifiConnected;
    }

    private NetworkInfo getWifiNetworkInfo() {
        ConnectivityManager mConnectivityManager = (ConnectivityManager) mContext.getApplicationContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetwork = mConnectivityManager.getActiveNetworkInfo();
        assert activeNetwork != null;
        if (activeNetwork.getType() == ConnectivityManager.TYPE_WIFI) {
            return activeNetwork;
        }
        return null;
    }
}
