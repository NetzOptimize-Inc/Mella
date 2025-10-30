package com.april.themellaapp.activity;

import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.april.themellaapp.R;
import com.google.firebase.auth.FirebaseAuth;

import java.util.Objects;

public class RegisterActivity extends AppCompatActivity {
    EditText mConfirmPassEditText, mPasswordEditText, mEmailEditText;
    TextView mSignupButton;
    TextView mLoginTextView, mForgotTextview;

    private FirebaseAuth firebaseAuth;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);
        initXMLComponents();

        firebaseAuth = FirebaseAuth.getInstance();
    }

    private void initXMLComponents() {
        mEmailEditText = findViewById(R.id.email_address_edit_text);
        mPasswordEditText = findViewById(R.id.password_edit_text);
        mSignupButton = findViewById(R.id.login_text_view);
        mLoginTextView = findViewById(R.id.register_text_view);
        mForgotTextview = findViewById(R.id.forgot_text_view);
        mForgotTextview.setVisibility(View.GONE);
        mConfirmPassEditText = findViewById(R.id.confirm_password_edit_text);
        mConfirmPassEditText.setVisibility(View.VISIBLE);
        mSignupButton.setText(R.string.register);
        mLoginTextView.setText(R.string.lbl_login);
        loginPageView();

        mSignupButton.setOnClickListener(v -> {
            String email = mEmailEditText.getText().toString().trim();
            String password = mPasswordEditText.getText().toString().trim();
            String confPassword = mConfirmPassEditText.getText().toString().trim();

            if (TextUtils.isEmpty(email)) {
                Toast.makeText(getApplicationContext(), "Enter email address!", Toast.LENGTH_SHORT).show();
                return;
            }

            if (TextUtils.isEmpty(password)) {
                Toast.makeText(getApplicationContext(), "Enter password!", Toast.LENGTH_SHORT).show();
                return;
            }

            if (TextUtils.isEmpty(confPassword)) {
                Toast.makeText(getApplicationContext(), "Enter password!", Toast.LENGTH_SHORT).show();
                return;
            }

            if (password.equals(confPassword)) {
                Toast.makeText(getApplicationContext(), "Password don't match!", Toast.LENGTH_SHORT).show();
                return;
            }

            if (password.length() < 6) {
                Toast.makeText(getApplicationContext(), "Password too short, enter minimum 6 characters!", Toast.LENGTH_SHORT).show();
                return;
            }

            firebaseAuth.createUserWithEmailAndPassword(email, password).addOnCompleteListener(RegisterActivity.this, task -> {
                if (!task.isSuccessful()) {
                    // If sign in fails, log the error message
                    Log.e("FirebaseAuthLogs", "Authentication failed.", task.getException());
                    Toast.makeText(RegisterActivity.this, "Authentication failed. " + Objects.requireNonNull(task.getException()).getMessage(), Toast.LENGTH_SHORT).show();
                } else {
                    // If sign in succeeds, log a success message
                    Log.d("FirebaseAuthLogs", "User registration successful.");
                    Toast.makeText(RegisterActivity.this, "Successfully Registered...", Toast.LENGTH_SHORT).show();

                    // Start the LoginActivity
                    startActivity(new Intent(RegisterActivity.this, LoginActivity.class));
                    finish(); // Finish the current activity
                }
            });
        });
    }

    private void loginPageView() {
        mLoginTextView.setOnClickListener(v -> {
            Intent intent = new Intent(RegisterActivity.this, LoginActivity.class);
            startActivity(intent);
            finish();
        });
    }
}
