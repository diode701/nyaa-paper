package com.nyar;
import com.nyar.config.Settings;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;


public class WpprLib {
    private List<Wppr> wpprLib;
    private Map<String, Wppr> idMap;
    private final Path workshopPath;
    public WpprLib(Path workshopPath){
        this.wpprLib = new ArrayList<>();
        this.idMap = new HashMap<>();
        this.workshopPath = workshopPath;

    }

    public void add(Wppr wallpaper){
        wpprLib.add(wallpaper);
        idMap.put(wallpaper.getId(), wallpaper);

    }

    public void addAll(List<Wppr> wallpapers){
        for (Wppr wp : wallpapers){
            add(wp);
        }
    }

    public List<Wppr> getAll(){
        return Collections.unmodifiableList(wpprLib);
    }

    public Wppr findById(String id){
        return idMap.get(id);
    }

    public void clear() {
        wpprLib.clear();
        idMap.clear();
    }

    //optional functions
    public List<Wppr> getPage(int page, int pageSize) {
        int start = page * pageSize;
        int end = Math.min(start + pageSize, wpprLib.size());

        if (start >= wpprLib.size()) {
            return new ArrayList<>();
        }

        return wpprLib.subList(start, end);
    }

    public int size() {
        return wpprLib.size();
    }

    public void sortByTitle() {
        wpprLib.sort((a, b) ->
                a.getTitle().compareToIgnoreCase(b.getTitle()));
    }

    public void refresh() {
        // Clear existing data
        wpprLib.clear();
        idMap.clear();

        // Load fresh data from workshop**
        List<Wppr> fresh = WpprRtrv.scanWorkshop(this.workshopPath);
        addAll(fresh);

        System.out.println("🔄 Refreshed: " + size() + " wallpapers");
    }
}
