package com.android.xo;

import android.app.Activity;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.*;

import java.io.File;


public class XoActivity extends Activity {

    XoView mView;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
    	
        DisplayMetrics metrics = new DisplayMetrics();
    	getWindowManager().getDefaultDisplay().getMetrics(metrics);
    	
        mView = new XoView(getApplication(), metrics.scaledDensity);
        setContentView(mView);
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }

    @Override protected void onDestroy() {
        super.onDestroy();
        XoLib.destroy( isFinishing() ? 0 : 1 );
    }
    
    @Override public boolean onTouchEvent( MotionEvent event ) {
        //Log.w( "a", "touch me " + Float.toString( event.getX() ) + ", " + Float.toString( event.getY() ) );
    	float[] x = new float[ event.getPointerCount() ];
    	float[] y = new float[ event.getPointerCount() ];
    	for ( int i = 0; i < event.getPointerCount(); i++ )
    	{
    		x[i] = event.getX(i);
    		y[i] = event.getY(i);
    	}
    	XoLib.input( 1, x, y );
    	mView.requestRender();
    	return true;
    }
}
