package com.april.themellaapp.activity;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.april.themellaapp.R;
import com.google.firebase.auth.FirebaseAuth;

public class ForgotPassword extends AppCompatActivity {
    TextView mForgotButton, mLoginTextView, mForgotTextview;
    FirebaseAuth firebaseAuth;
    EditText emailEdit, passwordEdit;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        // Get Firebase auth instance
        firebaseAuth = FirebaseAuth.getInstance();
        initXMLComponents();
    }

    private void initXMLComponents() {
        emailEdit = findViewById(R.id.email_address_edit_text);
        passwordEdit = findViewById(R.id.password_edit_text);

        mForgotButton = findViewById(R.id.login_text_view);
        mLoginTextView = findViewById(R.id.register_text_view);
        mForgotTextview = findViewById(R.id.forgot_text_view);

        mForgotButton.setText(R.string.send_email_link);
        mLoginTextView.setText(R.string.lbl_login);
        passwordEdit.setVisibility(View.GONE);
        mForgotTextview.setVisibility(View.GONE);

        loginPageView();
        sendLink();
    }

    private void loginPageView() {
        mLoginTextView.setOnClickListener(v -> {
            Intent intent = new Intent(ForgotPassword.this, LoginActivity.class);
            startActivity(intent);
            finish();
        });
    }

    private void sendLink() {
        mForgotButton.setOnClickListener(v -> {
            String email = emailEdit.getText().toString().trim();

            if (TextUtils.isEmpty(email)) {
                Toast.makeText(getApplication(), "Enter your registered email id", Toast.LENGTH_SHORT).show();
                return;
            }

            firebaseAuth.sendPasswordResetEmail(email).addOnCompleteListener(task -> {
                if (task.isSuccessful()) {
                    Toast.makeText(ForgotPassword.this, "We have sent you instructions to reset your password!", Toast.LENGTH_SHORT).show();
                    loginPageView();
                } else {
                    Toast.makeText(ForgotPassword.this, "Failed to send reset email!", Toast.LENGTH_SHORT).show();
                }

            });
        });
    }
}
