package com.nyar.wallpaper;

import com.nyar.settings.*;
import org.jetbrains.annotations.NotNull;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class WallpaperInstance {
    private final String monitor;
    private final String wallpaperId;
    private final List<Setting> settings;
    private final GpuConfig gpuConfig;
    private final String screenshotDir;      // can be null
    private final int screenshotDelay;        // only used if screenshotPath != null
    private final String screenshotName = "nyaa_sc.png";
    private Process process;

    public WallpaperInstance(String monitor, String wallpaperId, List<Setting> settings,
                             GpuConfig gpuConfig, String screenshotPath, int screenshotDelay) throws IOException {
        this.monitor = monitor;
        this.wallpaperId = wallpaperId;
        this.settings = new ArrayList<>(settings);
        this.gpuConfig = gpuConfig;
        this.screenshotDir = screenshotPath;
        this.screenshotDelay = screenshotDelay;
        start();
    }

    private void start() throws IOException {
        List<String> command = getStrings();

        // 1. Pre‑property flags (volume, fps, boolean flags, etc.)
        for (Setting s : settings) {
            if (s.isPrePropertyFlag()) {
                List<String> args = s.toCommandArgs();
                if (!args.isEmpty()) {
                    command.addAll(args);
                }
            }
        }

        // 2. Post‑property flags and custom properties (scaling, clamp, --set-property)
        for (Setting s : settings) {
            if (!s.isPrePropertyFlag()) {
                List<String> args = s.toCommandArgs();
                if (!args.isEmpty()) {
                    command.addAll(args);
                }
            }
        }

        System.out.println("Running command: " + String.join(" ", command));

        ProcessBuilder pb = new ProcessBuilder(command);

        // Apply GPU configuration
        switch (gpuConfig.getMode()) {
            case DRI_PRIME:
                pb.environment().put("DRI_PRIME", String.valueOf(gpuConfig.getDriPrimeNumber()));
                break;
            case NVIDIA:
                pb.environment().put("__NV_PRIME_RENDER_OFFLOAD", "1");
                pb.environment().put("__GLX_VENDOR_LIBRARY_NAME", "nvidia");
                break;
            // DEFAULT: nothing
        }

        if (gpuConfig.isGlThreadedOpt()) {
            pb.environment().put("__GL_THREADED_OPTIMIZATIONS", "0");
        }

        pb.inheritIO();
        process = pb.start();

        // Event-based crash detection — fires when the process exits on its own
        process.onExit().thenAccept(p -> {
            int code = p.exitValue();
            if (code != 0 && code != 143) { // 143 = SIGTERM from stop()
                WallpaperManager.getInstance().notifyCrash(monitor, code, WallpaperInstance.this);
            }
        });
    }

    private @NotNull List<String> getStrings() {
        List<String> command = new ArrayList<>();
        command.add("linux-wallpaperengine");

        // Screen and background
        command.add("--screen-root");
        command.add(monitor);
        command.add("--bg");
        command.add(wallpaperId);

        // Screenshot (if enabled)
        if (screenshotDir != null && !screenshotDir.isBlank()) {
            command.add("--screenshot");
            String sep = screenshotDir.endsWith("/") ? "" : "/";
            String screenshotPath = screenshotDir + sep + monitor + "_" + screenshotName;
            command.add(screenshotPath);
            command.add("--screenshot-delay");
            command.add(String.valueOf(screenshotDelay));
        }
        return command;
    }

    public void stop() {
       if (process == null || !process.isAlive()) return;

       ProcessHandle handle = process.toHandle();

       // Try graceful termination first
       handle.destroy();  // SIGTERM

       try {
           // Wait up to 3 seconds for graceful exit
           handle.onExit().get(3, TimeUnit.SECONDS);
           // Process exited cleanly
           process = null;
           return;
       } catch (InterruptedException | TimeoutException e) {
           // Timed out or interrupted – proceed to force kill
       } catch (ExecutionException e) {
           // Process exit caused an exception, but process is likely dead anyway
       }

       // Force kill the whole process tree
       handle.descendants().forEach(ProcessHandle::destroyForcibly);
       handle.destroyForcibly();

       try {
           handle.onExit().get(2, TimeUnit.SECONDS);
       } catch (Exception ignored) {}

       process = null;
   }

    public String getMonitor() { return monitor; }
    public String getWallpaperId() { return wallpaperId; }
    public List<Setting> getSettings() { return settings; }
}
