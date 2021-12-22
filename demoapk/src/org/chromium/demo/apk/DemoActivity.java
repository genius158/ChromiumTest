package org.chromium.demo.apk;

import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.util.AttributeSet;
import android.widget.TextView;

public class DemoActivity extends android.app.Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText("Test Test");
        tv.setTextColor(Color.RED);
        setContentView(tv);
    }
}
