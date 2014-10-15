package com.android.xo;

// Wrapper for native library

public class XoLib {

     static {
         System.loadLibrary("xo");
     }

     public static native void initXo(String cacheDir, float defaultDisplayScalingFactor);
     public static native void initSurface(int width, int height, float displayScalingFactor);
     public static native void surfacelost();
     public static native int render();
     public static native void destroy();

     public static native void input(int type, float[] x, float[] y);

}