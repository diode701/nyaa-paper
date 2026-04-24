package com.nyar;

import java.io.IOException;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class WpprRtrv {

 public static List<Wppr> scanWorkshop(Path workshopPath) {
    List<Wppr> found = new ArrayList<>();

    if (!Files.exists(workshopPath)) {
        System.out.println("Workshop path does not exist: " + workshopPath);
        return found;
    }

    try (DirectoryStream<Path> stream = Files.newDirectoryStream(workshopPath)) {
        for (Path folder : stream) {
            if (Files.isDirectory(folder)) {
                String folderName = folder.getFileName().toString();
                String basePath = folder.toString();  // Full path to folder
                Path jsonPath = folder.resolve("project.json");

                if (Files.exists(jsonPath)) {
                    String jsonContent = Files.readString(jsonPath);
                    Wppr wallpaper = ParseMD.fromJson(jsonContent, folderName, basePath);
                    if (wallpaper != null) {
                        found.add(wallpaper);
                    }
                }
            }
        }
    } catch (IOException e) {
        System.out.println("Couldn't read directory: " + e.getMessage());
    }
    return found;
}
}
