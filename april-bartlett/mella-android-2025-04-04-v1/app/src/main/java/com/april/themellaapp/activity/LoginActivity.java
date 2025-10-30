package com.april.themellaapp.activity;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.TextUtils;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.april.themellaapp.R;
import com.google.firebase.auth.FirebaseAuth;

public class LoginActivity extends AppCompatActivity {
    TextView mLoginTextView, mRegisterTextView, mForgotTextView;
    FirebaseAuth firebaseAuth;
    EditText emailEdit, passwordEdit;

    private String email, password;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        firebaseAuth = FirebaseAuth.getInstance();
        emailEdit = findViewById(R.id.email_address_edit_text);
        passwordEdit = findViewById(R.id.password_edit_text);

        initXMLComponents();
    }

    private void initXMLComponents() {
        mLoginTextView = findViewById(R.id.login_text_view);
        mForgotTextView = findViewById(R.id.forgot_text_view);

        mLoginTextView.setOnClickListener(view -> {
            email = emailEdit.getText().toString().trim();
            password = passwordEdit.getText().toString().trim();

            if (TextUtils.isEmpty(email)) {
                Toast.makeText(getApplicationContext(), "Enter email address!", Toast.LENGTH_SHORT).show();
                return;
            }

            if (TextUtils.isEmpty(password)) {
                Toast.makeText(getApplicationContext(), "Enter password!", Toast.LENGTH_SHORT).show();
                return;
            }

            if (password.length() < 6) {
                Toast.makeText(getApplicationContext(), "Password too short, enter minimum 6 characters!", Toast.LENGTH_SHORT).show();
                return;
            }

            // Authenticate User
            firebaseAuth.signInWithEmailAndPassword(email, password).addOnCompleteListener(LoginActivity.this, task -> {
                // If sign in fails, display a message to the user. If sign in succeeds
                // the auth state listener will be notified and logic to handle the
                // signed in user can be handled in the listener.
                if (!task.isSuccessful()) {
                    // there was an error
                    if (password.length() < 6) {
                        passwordEdit.setError(getString(R.string.minimum_password));
                    } else {
                        Toast.makeText(LoginActivity.this, getString(R.string.auth_failed), Toast.LENGTH_LONG).show();
                    }
                } else {
                    SharedPreferences sharedPreferences = getApplicationContext().getSharedPreferences("UserDetails", MODE_PRIVATE);
                    SharedPreferences.Editor editor = sharedPreferences.edit();
                    editor.putString("UserEmail", email);
                    editor.putString("UserPass", password);
                    editor.apply();

                    Intent intent = new Intent(LoginActivity.this, DeviceListActivity.class);
                    startActivity(intent);
                    finish();
                }
            });
        });

        mRegisterTextView = findViewById(R.id.register_text_view);
        mRegisterTextView.setOnClickListener(view -> {
            Intent intent = new Intent(LoginActivity.this, RegisterActivity.class);
            startActivity(intent);
        });

        mForgotTextView.setOnClickListener(v -> {
            Intent intent = new Intent(LoginActivity.this, ForgotPassword.class);
            startActivity(intent);
        });
    }
}
