package com.april.themellaapp.fragment;

import android.app.Dialog;
import android.app.TimePickerDialog;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.TimePicker;

import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;

import com.april.themellaapp.R;
import com.april.themellaapp.activity.NewTaskActivity;

import java.util.Calendar;
import java.util.Locale;

public class TimePickerFragment extends DialogFragment implements TimePickerDialog.OnTimeSetListener {
    static final int SUN_ON = 1, SUN_OFF = 2, MON_ON = 3, MON_OFF = 4, TUE_ON = 5, TUE_OFF = 6, WED_ON = 7, WED_OFF = 8, THUR_ON = 9, THUR_OFF = 10, FRI_ON = 11, FRI_OFF = 12, SAT_ON = 13, SAT_OFF = 14;
    int cur = 0;
    private int mChosenTimer = 0;

    public TimePickerFragment() {
        // Required empty public constructor
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final Calendar calendar = Calendar.getInstance();
        int hour = calendar.get(Calendar.HOUR_OF_DAY);
        int minute = calendar.get(Calendar.MINUTE);

        Bundle bundle = this.getArguments();
        if (bundle != null) {
            mChosenTimer = bundle.getInt("TIMER", 1);
        }

        switch (mChosenTimer) {
            case SUN_ON:
                cur = SUN_ON;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case SUN_OFF:
                cur = SUN_OFF;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case MON_ON:
                cur = MON_ON;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case MON_OFF:
                cur = MON_OFF;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case TUE_ON:
                cur = TUE_ON;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case TUE_OFF:
                cur = TUE_OFF;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case WED_ON:
                cur = WED_ON;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case WED_OFF:
                cur = WED_OFF;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case THUR_ON:
                cur = THUR_ON;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case THUR_OFF:
                cur = THUR_OFF;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case FRI_ON:
                cur = FRI_ON;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case FRI_OFF:
                cur = FRI_OFF;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case SAT_ON:
                cur = SAT_ON;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
            case SAT_OFF:
                cur = SAT_OFF;
                return new TimePickerDialog(getActivity(), android.R.style.Theme_Material_Light_Dialog_Alert, this, hour, minute, false);
        }
        return null;
    }

    @Override
    public void onTimeSet(TimePicker timePicker, int hours, int minutes) {
        if (getActivity() == null) {
            return;
        }

        String en = String.format(new Locale("en"), "%02d:%02d", hours, minutes);
        TextView mOnTimeTextView;
        TextView mOffTimeTextView;

        if (cur == SUN_ON) {
            mOnTimeTextView = getActivity().findViewById(R.id.sun_on_timer_text_view);
            mOnTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).sunOnTime = en;
            ((NewTaskActivity) getActivity()).sunStartHour = hours;
        } else if (cur == SUN_OFF) {
            mOffTimeTextView = getActivity().findViewById(R.id.sun_off_timer_text_view);
            mOffTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).sunOffTime = en;
            ((NewTaskActivity) getActivity()).sunEndHour = hours;
        } else if (cur == MON_ON) {
            mOnTimeTextView = getActivity().findViewById(R.id.mon_on_timer_text_view);
            mOnTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).monOnTime = en;
            ((NewTaskActivity) getActivity()).monStartHour = hours;
        } else if (cur == MON_OFF) {
            mOffTimeTextView = getActivity().findViewById(R.id.mon_off_timer_text_view);
            mOffTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).monOffTime = en;
            ((NewTaskActivity) getActivity()).monEndHour = hours;
        } else if (cur == TUE_ON) {
            mOnTimeTextView = getActivity().findViewById(R.id.tue_on_timer_text_view);
            mOnTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).tueOnTime = en;
            ((NewTaskActivity) getActivity()).tueStartHour = hours;
        } else if (cur == TUE_OFF) {
            mOffTimeTextView = getActivity().findViewById(R.id.tue_off_timer_text_view);
            mOffTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).tueOffTime = en;
            ((NewTaskActivity) getActivity()).tueEndHour = hours;
        } else if (cur == WED_ON) {
            mOnTimeTextView = getActivity().findViewById(R.id.wed_on_timer_text_view);
            mOnTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).wedOnTime = en;
            ((NewTaskActivity) getActivity()).wedStartHour = hours;
        } else if (cur == WED_OFF) {
            mOffTimeTextView = getActivity().findViewById(R.id.wed_off_timer_text_view);
            mOffTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).wedOffTime = en;
            ((NewTaskActivity) getActivity()).wedEndHour = hours;
        } else if (cur == THUR_ON) {
            mOnTimeTextView = getActivity().findViewById(R.id.thurs_on_timer_text_view);
            mOnTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).thurOnTime = en;
            ((NewTaskActivity) getActivity()).thuStartHour = hours;
        } else if (cur == THUR_OFF) {
            mOffTimeTextView = getActivity().findViewById(R.id.thurs_off_timer_text_view);
            mOffTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).thurOffTime = en;
            ((NewTaskActivity) getActivity()).thuEndHour = hours;
        } else if (cur == FRI_ON) {
            mOnTimeTextView = getActivity().findViewById(R.id.fri_on_timer_text_view);
            mOnTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).friOnTime = en;
            ((NewTaskActivity) getActivity()).friStartHour = hours;
        } else if (cur == FRI_OFF) {
            mOffTimeTextView = getActivity().findViewById(R.id.fri_off_timer_text_view);
            mOffTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).friOffTime = en;
            ((NewTaskActivity) getActivity()).friEndHour = hours;
        } else if (cur == SAT_ON) {
            mOnTimeTextView = getActivity().findViewById(R.id.sat_on_timer_text_view);
            mOnTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).satOnTime = en;
            ((NewTaskActivity) getActivity()).satStartHour = hours;
        } else if (cur == SAT_OFF) {
            mOffTimeTextView = getActivity().findViewById(R.id.sat_off_timer_text_view);
            mOffTimeTextView.setText(en);
            ((NewTaskActivity) getActivity()).satOffTime = en;
            ((NewTaskActivity) getActivity()).satEndHour = hours;
        }
    }
}
