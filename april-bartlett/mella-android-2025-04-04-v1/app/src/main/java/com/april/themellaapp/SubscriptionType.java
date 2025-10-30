package com.april.themellaapp;

import androidx.annotation.NonNull;

public enum SubscriptionType {
    NONE(-1),
    MONTHLY(0),
    ANNUALLY(1);

    final int value;

    SubscriptionType(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    @NonNull
    @Override
    public String toString() {
        return String.valueOf(value);
    }
}
