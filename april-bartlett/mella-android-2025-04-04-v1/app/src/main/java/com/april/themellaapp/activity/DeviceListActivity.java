package com.april.themellaapp.activity;

import android.app.Dialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.ImageView;
import android.widget.RelativeLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.april.themellaapp.ListItem;
import com.april.themellaapp.R;
import com.april.themellaapp.adapter.DataAdapter;
import com.april.themellaapp.utils.FirebaseDatabaseReference;
import com.google.firebase.database.ChildEventListener;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;

import java.util.ArrayList;
import java.util.List;

public class DeviceListActivity extends AppCompatActivity {
    RelativeLayout mRelativeLayout;
    ImageView logoutView;
    DatabaseReference databaseReference;
    SharedPreferences sharedPreferences;
    SharedPreferences.Editor editor;

    private RecyclerView recyclerView;
    private DataAdapter adapter;
    private List<ListItem> listItems;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_list);
        initXMLComponents();
    }

    private void initXMLComponents() {
        logoutView = findViewById(R.id.logout);

        recyclerView = findViewById(R.id.recycler_view);
        mRelativeLayout = findViewById(R.id.relative_lyt);
        mRelativeLayout.setOnClickListener(view -> {
            Intent intent = new Intent(DeviceListActivity.this, NewDeviceActivity.class);
            startActivity(intent);
        });

        sharedPreferences = getApplicationContext().getSharedPreferences("UserDetails", MODE_PRIVATE);
        editor = sharedPreferences.edit();

        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        listItems = new ArrayList<>();

        databaseReference = FirebaseDatabaseReference.deviceRef();
        databaseReference.addChildEventListener(new ChildEventListener() {
            @Override
            public void onChildAdded(@NonNull DataSnapshot dataSnapshot, @Nullable String s) {
                String string = dataSnapshot.getValue(String.class);
                String devID = dataSnapshot.getKey();
                ListItem listItem = new ListItem(string, devID);

                listItems.add(listItem);
                adapter = new DataAdapter(listItems, DeviceListActivity.this);
                recyclerView.setAdapter(adapter);
            }

            @Override
            public void onChildChanged(@NonNull DataSnapshot dataSnapshot, @Nullable String s) {

            }

            @Override
            public void onChildRemoved(@NonNull DataSnapshot dataSnapshot) {
                dataSnapshot.getKey();
            }

            @Override
            public void onChildMoved(@NonNull DataSnapshot dataSnapshot, @Nullable String s) {

            }

            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {

            }
        });

        // Logout from deviceActivity
        logoutView.setOnClickListener(v -> logoutView.setOnClickListener(v1 -> {
            AlertDialog alertDialog = new AlertDialog.Builder(DeviceListActivity.this).create();
            alertDialog.setTitle("Alert Dialog");
            alertDialog.setMessage("Do you really want to logout from this device ?");
            alertDialog.setIcon(R.drawable.warning);
            alertDialog.setButton(Dialog.BUTTON_NEGATIVE, "Cancel", (dialog, which) -> {
                // nothing here
            });
            alertDialog.setButton(Dialog.BUTTON_POSITIVE, "OK", (dialog, which) -> {
                editor.putString("UserEmail", "");
                editor.putString("UserPass", "");
                editor.putString("DevID", "");
                editor.apply();

                Intent intent = new Intent(DeviceListActivity.this, LoginActivity.class);
                startActivity(intent);
                finish();
            });

            // Showing Alert Message
            alertDialog.show();
        }));
    }
}
