package com.april.themellaapp.adapter;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.recyclerview.widget.RecyclerView;

import com.april.themellaapp.ListItem;
import com.april.themellaapp.R;
import com.april.themellaapp.activity.DeviceListActivity;
import com.april.themellaapp.activity.MainActivity;
import com.april.themellaapp.activity.NewDeviceActivity;
import com.april.themellaapp.utils.FirebaseDatabaseReference;
import com.google.firebase.database.DatabaseReference;

import java.util.List;

public class DataAdapter extends RecyclerView.Adapter<DataAdapter.ViewHolder> {
    private final List<ListItem> listItems;
    private final Context context;

    String deviceID;
    String deviceName;

    public DataAdapter(List<ListItem> listItems, Context context) {
        this.listItems = listItems;
        this.context = context;
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View v = LayoutInflater.from(parent.getContext()).inflate(R.layout.device_list_item, parent, false);
        return new ViewHolder(v);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, final int position) {
        holder.nameTextView.setText(listItems.get(position).getDesc());
        holder.linearLayout.setOnClickListener(v -> {
            deviceID = listItems.get(position).getHead();
            Intent intent = new Intent(context, MainActivity.class);

            // Passing data to main activity
            intent.putExtra("DevID", deviceID);
            context.startActivity(intent);
        });
        holder.deleteImageView.setOnClickListener(v -> {
            AlertDialog alertDialog = new AlertDialog.Builder(context).create();
            alertDialog.setTitle("Alert Dialog");
            alertDialog.setMessage("Do you really want to remove this device from the list ?");
            alertDialog.setIcon(R.drawable.warning);
            alertDialog.setButton(Dialog.BUTTON_NEGATIVE, "Cancel", (dialog, which) -> {
                // empty
            });
            alertDialog.setButton(Dialog.BUTTON_POSITIVE, "OK", (dialog, which) -> {
                deviceID = listItems.get(position).getDesc();
                deleteDevice(deviceID);
            });
            alertDialog.show();
        });

        holder.settingImageView.setOnClickListener(v -> {
            deviceID = listItems.get(position).getHead();
            deviceName = listItems.get(position).getDesc();

            Intent intent = new Intent(context, NewDeviceActivity.class);
            intent.putExtra("DevName", deviceName);
            intent.putExtra("DevID", deviceID);
            context.startActivity(intent);
        });
    }

    @Override
    public int getItemCount() {
        return listItems.size();
    }

    private void deleteDevice(String devID) {
        DatabaseReference drDevice = FirebaseDatabaseReference.deviceRef().child(devID);
        drDevice.removeValue();
        Toast.makeText(context, "Item deleted.", Toast.LENGTH_SHORT).show();
        Intent intent = new Intent(context, DeviceListActivity.class);
        context.startActivity(intent);
    }

    public static class ViewHolder extends RecyclerView.ViewHolder {
        public TextView nameTextView;
        public ImageView deleteImageView;
        public ImageView settingImageView;
        LinearLayout linearLayout;

        public ViewHolder(View itemView) {
            super(itemView);

            nameTextView = itemView.findViewById(R.id.device_name_text_view);
            linearLayout = itemView.findViewById(R.id.device_linear_lyt);
            deleteImageView = itemView.findViewById(R.id.delete_dev);
            settingImageView = itemView.findViewById(R.id.setting_dev);
        }
    }
}
