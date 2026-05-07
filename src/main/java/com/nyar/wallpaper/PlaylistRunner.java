package com.nyar.wallpaper;

import com.nyar.Wppr;
import com.nyar.WpprLib;
import com.nyar.config.Playlist;
import com.nyar.config.PlaylistManager;
import com.nyar.config.WallpaperConfig;
import com.nyar.settings.*;
import com.nyar.utils.VideoDurationUtil;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.concurrent.*;
import java.util.function.BiConsumer;

public class PlaylistRunner {
    private static final PlaylistRunner INSTANCE = new PlaylistRunner();

    private final PlaylistManager playlistManager = PlaylistManager.getInstance();
    private final WallpaperManager wallpaperManager = WallpaperManager.getInstance();
    private final Map<String, PlaybackState> states = new ConcurrentHashMap<>();
    private ScheduledExecutorService scheduler;
    private BiConsumer<String, String> statusListener;

    private static class PlaybackState {
        ScheduledFuture<?> timerTask;
        Playlist playlist;
        boolean paused;
    }

    private PlaylistRunner() {}

    public static PlaylistRunner getInstance() { return INSTANCE; }

    public void setStatusListener(BiConsumer<String, String> listener) {
        this.statusListener = listener;
    }

    private ScheduledExecutorService getScheduler() {
        if (scheduler == null || scheduler.isShutdown()) {
            scheduler = Executors.newSingleThreadScheduledExecutor(r -> {
                Thread t = new Thread(r, "playlist-runner");
                t.setDaemon(true);
                return t;
            });
        }
        return scheduler;
    }

    public void play(String monitor) {
        PlaybackState state = states.get(monitor);
        // If already playing (active and not paused), ignore.
        if (state != null && !state.paused) {
            fireStatus(monitor, "Already playing");
            return;
        }

        // If there is an existing paused state, resume it to avoid index loss
        if (state != null && state.paused) {
            resume(monitor);
            return;
        }

        // Load playlist from disk
        Playlist playlist = playlistManager.load(monitor);
        if (playlist == null || playlist.getEntries().isEmpty()) {
            fireStatus(monitor, "Playlist is empty");
            return;
        }

        // Remove entries whose wallpapers no longer exist
        cleanupMissing(playlist);
        if (playlist.getEntries().isEmpty()) {
            playlistManager.save(monitor, playlist);
            fireStatus(monitor, "Playlist is empty");
            return;
        }
        playlistManager.save(monitor, playlist);

        // Cancel any lingering timer (shouldn't exist)
        cancelTimer(state);

        PlaybackState newState = new PlaybackState();
        newState.playlist = playlist;
        newState.paused = false;
        states.put(monitor, newState);

        // Ensure index is valid
        if (playlist.getCurrentIndex() < 0 || playlist.getCurrentIndex() >= playlist.getEntries().size()) {
            playlist.setCurrentIndex(0);
        }

        applyEntryAndSchedule(monitor, newState);
        fireStatus(monitor, "Playing");
    }

    public void pause(String monitor) {
        PlaybackState state = states.get(monitor);
        if (state == null || state.paused) return;
        cancelTimer(state);
        state.paused = true;
        playlistManager.save(monitor, state.playlist);
        fireStatus(monitor, "Paused");
    }

    public void resume(String monitor) {
        PlaybackState state = states.get(monitor);
        if (state == null || !state.paused) return;
        state.paused = false;
        // Resume from the current index without reapplying (the entry is still on screen)
        scheduleNext(monitor, state);
        fireStatus(monitor, "Resumed");
    }

    public void stop(String monitor) {
        PlaybackState state = states.remove(monitor);
        if (state == null) return;
        cancelTimer(state);
        wallpaperManager.stop(monitor);
        fireStatus(monitor, "Stopped");
    }

    public void next(String monitor) {
        PlaybackState state = states.get(monitor);
        if (state == null || state.playlist == null) return;
        cancelTimer(state);
        if (!advance(state, +1)) {
            fireStatus(monitor, "End of playlist");
            stop(monitor);
            return;
        }
        playlistManager.save(monitor, state.playlist);
        applyEntryAndSchedule(monitor, state);
        fireStatus(monitor, "Next");
    }

    public void prev(String monitor) {
        PlaybackState state = states.get(monitor);
        if (state == null || state.playlist == null) return;
        cancelTimer(state);
        advance(state, -1);
        playlistManager.save(monitor, state.playlist);
        applyEntryAndSchedule(monitor, state);
        fireStatus(monitor, "Previous");
    }

    public void reset(String monitor) {
        PlaybackState state = states.get(monitor);
        if (state == null || state.playlist == null) return;
        cancelTimer(state);
        state.playlist.setCurrentIndex(0);
        playlistManager.save(monitor, state.playlist);
        applyEntryAndSchedule(monitor, state);
        fireStatus(monitor, "Reset");
    }

    public void reloadPlaylist(String monitor) {
        PlaybackState state = states.get(monitor);
        if (state == null) return;
        Playlist playlist = playlistManager.load(monitor);
        if (playlist == null || playlist.getEntries().isEmpty()) {
            stop(monitor);
            return;
        }
        state.playlist = playlist;
        if (playlist.getCurrentIndex() < 0 || playlist.getCurrentIndex() >= playlist.getEntries().size()) {
            playlist.setCurrentIndex(0);
        }
    }

    public boolean isPlaying(String monitor) {
        PlaybackState state = states.get(monitor);
        return state != null && !state.paused;
    }

    public boolean isPaused(String monitor) {
        PlaybackState state = states.get(monitor);
        return state != null && state.paused;
    }

    public boolean isActive(String monitor) {
        return states.containsKey(monitor);
    }

    public Playlist getActivePlaylist(String monitor) {
        PlaybackState state = states.get(monitor);
        return state != null ? state.playlist : null;
    }

    public int getCurrentIndex(String monitor) {
        PlaybackState state = states.get(monitor);
        return (state != null && state.playlist != null) ? state.playlist.getCurrentIndex() : -1;
    }

    // ----------------------------------------------------------------------

    private void applyEntryAndSchedule(String monitor, PlaybackState state) {
        List<WallpaperConfig> entries = state.playlist.getEntries();
        WpprLib lib = WpprLib.getInstance();

        // Walk forward until we find an entry whose wallpaper actually exists
        int maxAttempts = entries.size();
        for (int attempt = 0; attempt < maxAttempts; attempt++) {
            int idx = state.playlist.getCurrentIndex();
            if (idx < 0 || idx >= entries.size()) {
                state.playlist.setCurrentIndex(0);
                idx = 0;
            }

            WallpaperConfig cfg = entries.get(idx);
            if (lib == null) {
                System.out.println("PlaylistRunner: WpprLib not available, skipping " + cfg.getWallpaperId());
                if (!advance(state, +1)) break;
                continue;
            }

            Wppr wp = lib.findById(cfg.getWallpaperId());
            if (wp == null) {
                System.out.println("PlaylistRunner: wallpaper " + cfg.getWallpaperId() + " not found, removing from playlist");
                entries.remove(idx);
                playlistManager.save(monitor, state.playlist);
                continue;
            }

            // Build GPU config
            GpuConfig gpu = new GpuConfig();
            try {
                gpu.setMode(GpuConfig.Mode.valueOf(cfg.getGpuMode()));
            } catch (IllegalArgumentException e) {
                gpu.setMode(GpuConfig.Mode.DEFAULT);
            }
            gpu.setDriPrimeNumber(cfg.getDriPrimeNumber());
            gpu.setGlThreadedOpt(cfg.isGlThreadedOpt());

            List<Setting> settings = buildSettingsFromConfig(cfg, wp);

            try {
                wallpaperManager.apply(monitor, cfg.getWallpaperId(), settings, gpu, null, 0);
                System.out.println("PlaylistRunner: applied " + cfg.getWallpaperId() + " on " + monitor);
                playlistManager.save(monitor, state.playlist);
                if (!state.paused) {
                    scheduleNext(monitor, state);
                }
                return;
            } catch (Exception e) {
                System.out.println("PlaylistRunner: failed to apply " + cfg.getWallpaperId() + ": " + e.getMessage());
                if (!advance(state, +1)) break;
            }
        }

        // Every entry was invalid or failed — stop playback
        System.out.println("PlaylistRunner: no valid entries left for " + monitor + ", stopping");
        stop(monitor);
    }

    private void cleanupMissing(Playlist playlist) {
        WpprLib lib = WpprLib.getInstance();
        if (lib == null) return;
        playlist.getEntries().removeIf(cfg -> lib.findById(cfg.getWallpaperId()) == null);
    }

    private void scheduleNext(String monitor, PlaybackState state) {
        if (state.paused) return;

        List<WallpaperConfig> entries = state.playlist.getEntries();
        int idx = state.playlist.getCurrentIndex();
        if (idx < 0 || idx >= entries.size()) return;

        WallpaperConfig cfg = entries.get(idx);
        double durationSecs = getDuration(cfg, state.playlist.getIntervalSec());

        state.timerTask = getScheduler().schedule(() -> {
            PlaybackState s = states.get(monitor);
            if (s == null || s.paused) return;
            if (!advance(s, +1)) {
                fireStatus(monitor, "Playlist finished");
                stop(monitor);
                return;
            }
            playlistManager.save(monitor, s.playlist);
            applyEntryAndSchedule(monitor, s);
        }, (long) (durationSecs * 1000), TimeUnit.MILLISECONDS);
    }

      private double getDuration(WallpaperConfig cfg, int fallbackSec) {
        WpprLib lib = WpprLib.getInstance();
        if (lib == null) return fallbackSec;

        Wppr wp = lib.findById(cfg.getWallpaperId());
        if (wp == null) return fallbackSec;

        if ("video".equalsIgnoreCase(wp.getType())) {
            // Use the file field from project.json if it's a real filename
            String fileName = wp.getFile();
            if (fileName != null && !fileName.isBlank() && !"unknown file".equals(fileName)) {
                Path videoPath = Path.of(wp.getBasePath(), fileName);
                if (Files.exists(videoPath)) {
                    double dur = VideoDurationUtil.checkVideoDuration(videoPath);
                    if (dur > 0) return dur;
                }
            }
        }
        return fallbackSec;
    }

    private boolean advance(PlaybackState state, int delta) {
        List<WallpaperConfig> entries = state.playlist.getEntries();
        if (entries.isEmpty()) return false;

        if (state.playlist.getMode() == Playlist.Mode.RANDOM) {
            int next = new Random().nextInt(entries.size());
            state.playlist.setCurrentIndex(next);
            return true;
        } else {
            int newIdx = state.playlist.getCurrentIndex() + delta;
            if (newIdx < 0) {
                newIdx = entries.size() - 1;
            }
            if (newIdx >= entries.size()) {
                if (!state.playlist.isLoop()) return false;
                newIdx = 0;
            }
            state.playlist.setCurrentIndex(newIdx);
            return true;
        }
    }

    private void cancelTimer(PlaybackState state) {
        if (state != null && state.timerTask != null) {
            state.timerTask.cancel(false);
            state.timerTask = null;
        }
    }

    private void fireStatus(String monitor, String status) {
        if (statusListener != null) {
            statusListener.accept(monitor, status);
        }
    }

    List<Setting> buildSettingsFromConfig(WallpaperConfig cfg, Wppr wp) {
        List<Setting> defaults = PropertyParser.parseFromWppr(wp);
        Map<String, String> saved = cfg.getSettings();
        List<Setting> all = new ArrayList<>(defaults);
        for (Setting s : all) {
            String val = saved.get(s.getName());
            if (val != null) s.setValueFromString(val);
        }
        all.addAll(buildGlobalOptionsFromConfig(saved));
        return all;
    }

    private List<Setting> buildGlobalOptionsFromConfig(Map<String, String> saved) {
        List<GlobalOption> globals = GlobalOptionsFactory.createAll();
        List<Setting> result = new ArrayList<>();
        for (Setting g : globals) {
            String val = saved.get(g.getName());
            if (val != null) g.setValueFromString(val);
            result.add(g);
        }
        return result;
    }
}