package com.nyar;

import com.nyar.config.Settings;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class Builder {
    private List<Wppr> wallpapers;
    private Settings settings;
    private Path activeWorkshopPath; // Keep track of the resolved path

    public Builder(Settings settings) {
        this.settings = settings;
        this.wallpapers = new ArrayList<>();

        // Resolve the path immediately based on the settings passed in
        String pathStr = settings.getSteamPath();
        if (pathStr == null || pathStr.isEmpty()) {
            pathStr = System.getProperty("user.home") + "/.local/share/Steam/steamapps/workshop/content/431960/";
        }
        this.activeWorkshopPath = Paths.get(pathStr);
    }

    public Builder scanWorkshop() {
        // Pass the path to the scanner
        List<Wppr> found = WpprRtrv.scanWorkshop(activeWorkshopPath);
        wallpapers.addAll(found);
        return this;
    }

    public WpprLib build() {
        WpprLib lib = new WpprLib(activeWorkshopPath);
        lib.addAll(wallpapers);
        return lib;
    }

    // New helper method to construct the watcher easily
    public WorkshopWatcher buildWatcher(WpprLib lib, Runnable onChange) throws IOException {
        // Pass the path to the watcher
        return new WorkshopWatcher(lib, activeWorkshopPath, onChange);
    }
}
