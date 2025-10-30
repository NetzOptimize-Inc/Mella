package com.april.themellaapp.utils;

import com.google.firebase.auth.FirebaseAuth;
import com.google.firebase.auth.FirebaseUser;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;

public class FirebaseDatabaseReference {

    public static DatabaseReference units() {
        FirebaseDatabase database = FirebaseDatabase.getInstance();
        return database.getReference("Units");
    }

    public static DatabaseReference deviceRef() {
        FirebaseUser firUser = FirebaseAuth.getInstance().getCurrentUser();
        if (firUser == null) {
            return units().child("unknown");
        }
        return units().child(firUser.getUid());
    }
}
