plugins {
    java
    application
    id("org.openjfx.javafxplugin") version "0.0.14"
    id("com.gradleup.shadow") version "8.3.6"
}

group = "com.nyar"
version = "1.0"

repositories {
    mavenCentral()
}

javafx {
    version = "21"
    modules("javafx.controls", "javafx.fxml")
}

dependencies {
    implementation("org.json:json:20230618")
    implementation("com.dorkbox:SystemTray:4.3")
    implementation("org.slf4j:slf4j-simple:2.0.9")

    val javafxModules = listOf("javafx-controls", "javafx-fxml", "javafx-graphics", "javafx-base")
    javafxModules.forEach { module ->
        implementation("org.openjfx:$module:21")
    }
}

tasks.withType<com.github.jengelman.gradle.plugins.shadow.tasks.ShadowJar> {
    archiveClassifier.set("")
    archiveVersion.set("")
    destinationDirectory.set(layout.buildDirectory.dir("libs"))
    mergeServiceFiles()
    exclude("module-info.class") // Crucial for JavaFX fat JARs
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
}

tasks.register<com.github.jengelman.gradle.plugins.shadow.tasks.ShadowJar>("appShadowJar") {
    archiveBaseName.set("nyaa-paper-app")
    manifest {
        attributes["Main-Class"] = "com.nyar.Main"
    }
    from(sourceSets.main.get().output)
    configurations = listOf(project.configurations.runtimeClasspath.get())
}

tasks.register<com.github.jengelman.gradle.plugins.shadow.tasks.ShadowJar>("launcherShadowJar") {
    archiveBaseName.set("nyaa-paper-launcher")
    manifest {
        attributes["Main-Class"] = "com.nyar.launcher.Main"
    }
    from(sourceSets.main.get().output)
    configurations = listOf(project.configurations.runtimeClasspath.get())
}

tasks.register("buildAll") {
    dependsOn("appShadowJar", "launcherShadowJar")
}

tasks.test {
    useJUnitPlatform()
}

application {
    mainClass.set("com.nyar.launcher.Main")
}
