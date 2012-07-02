package com.android.nudom;

// Wrapper for native library

public class NuLib {

     static {
         System.loadLibrary("nudom");
     }

    /**
     * @param width the current view width
     * @param height the current view height
     */
     public static native void init(int width, int height);
     public static native void surfacelost();
     public static native int step();
     public static native void destroy(int iskilling);

     public static native void input(int type, float[] x, float[] y);

}