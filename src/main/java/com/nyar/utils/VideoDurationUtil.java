package com.nyar.utils;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.TimeUnit;

public class VideoDurationUtil {

    /**
     * Returns the duration in seconds of the given video file.
     * Uses ffprobe with a 5‑second timeout. Returns -1 if the file
     * doesn't exist, ffprobe is not available, or any error occurs.
     */
    public static double checkVideoDuration(Path videoFile) {
        if (videoFile == null || !Files.exists(videoFile)) {
            return -1.0;
        }

        try {
            ProcessBuilder pb = new ProcessBuilder(
                    "ffprobe",
                    "-v", "error",
                    "-show_entries", "format=duration",
                    "-of", "default=noprint_wrappers=1:nokey=1",
                    videoFile.toString()
            );
            pb.redirectErrorStream(true);
            Process proc = pb.start();

            boolean finished = proc.waitFor(5, TimeUnit.SECONDS);
            if (!finished) {
                proc.destroyForcibly();
                System.out.println("ffprobe timed out for " + videoFile);
                return -1.0;
            }

            int exitCode = proc.exitValue();
            if (exitCode != 0) {
                System.out.println("ffprobe exited with code " + exitCode + " for " + videoFile);
                return -1.0;
            }

            try (BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream()))) {
                String line = reader.readLine();
                if (line != null && !line.isBlank()) {
                    return Double.parseDouble(line.trim());
                }
            } catch (NumberFormatException e) {
                System.out.println("ffprobe output not a number for " + videoFile + ": " + e.getMessage());
            }
        } catch (Exception e) {
            System.out.println("Failed to get video duration for " + videoFile + ": " + e.getMessage());
        }

        return -1.0;
    }
}
