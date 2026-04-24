package com.nyar.wallpaper;

public class GpuConfig {
    public enum Mode {
        DEFAULT,
        DRI_PRIME,
        NVIDIA
    }

    private Mode mode = Mode.DEFAULT;
    private int driPrimeNumber = 1;

    public GpuConfig() {}

    public Mode getMode() { return mode; }
    public void setMode(Mode mode) { this.mode = mode; }

    public int getDriPrimeNumber() { return driPrimeNumber; }
    public void setDriPrimeNumber(int driPrimeNumber) { this.driPrimeNumber = driPrimeNumber; }
}
