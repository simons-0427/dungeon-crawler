package com.dungeoncrawler.playfree;

import android.app.NativeActivity;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

/** Keeps the native raylib game in Android's immersive fullscreen mode. */
@SuppressWarnings("deprecation")
public final class FullscreenNativeActivity extends NativeActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN
                        | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams attributes = getWindow().getAttributes();
            attributes.layoutInDisplayCutoutMode =
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(attributes);
        }
        if (Build.VERSION.SDK_INT >= 33) {
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                    android.window.OnBackInvokedDispatcher.PRIORITY_DEFAULT,
                    this::queueGameBack);
        }
        setImmersive(true);
        requestImmersiveMode();
    }

    private boolean gameBackRequested;

    private synchronized void queueGameBack() { gameBackRequested = true; }

    public synchronized boolean consumeGameBack() {
        boolean result = gameBackRequested;
        gameBackRequested = false;
        return result;
    }

    @Override
    // Legacy button fallback; API 33+ gestures use the dispatcher registered above.
    @android.annotation.SuppressLint("GestureBackNavigation")
    public void onBackPressed() { queueGameBack(); }

    @Override
    protected void onResume() {
        super.onResume();
        requestImmersiveMode();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) requestImmersiveMode();
    }

    private void requestImmersiveMode() {
        final View decorView = getWindow().getDecorView();
        decorView.post(new Runnable() {
            @Override
            public void run() {
                hideSystemBars();
            }
        });
    }

    private void hideSystemBars() {
        Window window = getWindow();
        View decorView = window.getDecorView();
        setImmersive(true);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController controller = window.getInsetsController();
            if (controller != null) {
                controller.setSystemBarsBehavior(
                        WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                controller.hide(WindowInsets.Type.systemBars());
            }
        } else {
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN);
        }
    }
    private static native void onSeedEntered(String value);

    /** Seed entry uses Android's keyboard and returns to the native game. */
    public void requestSeedInput(final String initial) {
        runOnUiThread(() -> {
            final android.widget.EditText input = new android.widget.EditText(this);
            input.setSingleLine(true);
            input.setInputType(android.text.InputType.TYPE_CLASS_TEXT
                    | android.text.InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
            android.text.InputFilter ascii = (source, start, end, dest, dstart, dend) -> {
                StringBuilder accepted = new StringBuilder();
                for (int i = start; i < end; i++) {
                    char c = source.charAt(i);
                    if (c >= 32 && c <= 126) accepted.append(c);
                }
                return accepted.length() == end-start ? null : accepted.toString();
            };
            input.setFilters(new android.text.InputFilter[]{ascii, new android.text.InputFilter.LengthFilter(24)});
            input.setText(initial);
            input.setSelection(input.length());
            android.app.AlertDialog dialog = new android.app.AlertDialog.Builder(this)
                    .setTitle("Custom seed")
                    .setMessage("Up to 24 letters, numbers or symbols")
                    .setView(input)
                    .setPositiveButton("OK", (d, which) -> onSeedEntered(input.getText().toString()))
                    .setNegativeButton("Cancel", null)
                    .create();
            dialog.setOnDismissListener(d -> requestImmersiveMode());
            dialog.setOnShowListener(d -> {
                input.requestFocus();
                dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
            });
            dialog.show();
        });
    }
}
