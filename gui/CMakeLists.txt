cmake_minimum_required(VERSION 3.21)
project(nyaa-paper VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Quick Qml QuickControls2 DBus)
qt_standard_project_setup(REQUIRES 6.5)

set(INTERFACES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces)
qt_add_dbus_interface(GUI_DBUS_SOURCES ${INTERFACES_DIR}/org.nyaa.Paper.Engine.xml   engine_proxy)
qt_add_dbus_interface(GUI_DBUS_SOURCES ${INTERFACES_DIR}/org.nyaa.Paper.Library.xml  library_proxy)
qt_add_dbus_interface(GUI_DBUS_SOURCES ${INTERFACES_DIR}/org.nyaa.Paper.Playlist.xml playlist_proxy)

qt_add_executable(nyaa-paper-qt
    src/main.cpp
    src/domain/settings/Setting.cpp
    src/domain/settings/SettingFactory.cpp
    src/domain/settings/SettingList.cpp
    src/domain/Renderer.cpp
    src/domain/options/GlobalOptions.cpp
    src/io/ProjectJson.cpp
    src/util/VideoDuration.cpp
    src/io/ConfigStore.cpp
    src/io/WallpaperStore.cpp
    src/domain/settings/Settings.cpp
    src/engine/IEngine.hpp
    src/engine/DbusEngine.hpp
    src/engine/DbusEngine.cpp
    src/engine/MockEngine.hpp
    src/bridge/ConfigBridge.hpp
    src/bridge/ConfigBridge.cpp
    src/bridge/PlaylistBridge.hpp
    src/bridge/PlaylistBridge.cpp
    src/bridge/EngineBridge.hpp
    src/bridge/EngineBridge.cpp
    src/bridge/PropertiesController.hpp
    src/bridge/PropertiesController.cpp
    src/bridge/ConfigRenderer.hpp
    src/bridge/ConfigRenderer.cpp
    src/app/Autostart.hpp
    src/app/Autostart.cpp
    src/app/DevHooks.hpp
    src/app/DevHooks.cpp
    src/app/StartupRestore.hpp
    src/app/StartupRestore.cpp
    src/app/Tray.hpp
    src/app/Tray.cpp
    src/util/Gpu.h
    src/util/Gpu.cpp
    src/io/WallpaperConfigJson.h
    src/io/WallpaperConfigJson.cpp
    src/domain/WallpaperConfig.h
    src/models/WallpaperModel.hpp
    src/models/WallpaperModel.cpp
    src/models/MockLibrary.hpp
    src/models/MockLibrary.cpp
    src/models/GlobalOptionsModel.hpp
    src/models/GlobalOptionsModel.cpp
    src/models/PropertyModel.hpp
    src/models/PropertyModel.cpp
    src/models/DbusLibrary.hpp
    src/models/DbusLibrary.cpp
    src/ui/Theme.hpp
    src/ui/PreviewSettings.hpp
    src/ui/PreviewSettings.cpp
    ${GUI_DBUS_SOURCES}
        src/domain/settings/Setting.h
        src/domain/settings/Setting.cpp
    src/domain/settings/SettingFactory.cpp
    src/domain/settings/SettingList.cpp
    src/domain/Renderer.cpp
)

qt_policy(SET QTP0004 NEW)

set_source_files_properties(qml/Look.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(nyaa-paper-qt
    URI NyaaPaper
    VERSION 1.0
    QML_FILES
        qml/Look.qml
        qml/Main.qml
        qml/views/BrowserView.qml
        qml/views/PropertiesView.qml
        qml/views/PlaylistView.qml
        qml/views/SettingsView.qml
        qml/components/ActionButton.qml
        qml/components/StatusBar.qml
        qml/components/Cell.qml
        qml/components/HintBar.qml
        qml/components/KeyCap.qml
        qml/components/MonitorPill.qml
        qml/components/Toast.qml
        qml/controls/SettingCard.qml
        qml/controls/BoolControl.qml
        qml/controls/SliderControl.qml
        qml/controls/ColorControl.qml
        qml/controls/ComboControl.qml
        qml/controls/TextControl.qml
        qml/controls/GroupControl.qml
        qml/controls/TextInputControl.qml
        qml/controls/PathControl.qml
        qml/controls/FileControl.qml
        qml/controls/DirectoryControl.qml
        qml/controls/TextureControl.qml
        qml/controls/VolumeControl.qml
        qml/controls/FpsControl.qml
        qml/controls/ScalingControl.qml
        qml/controls/ClampControl.qml
        qml/controls/FlagControl.qml
)

target_include_directories(nyaa-paper-qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(nyaa-paper-qt PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::DBus
)

qt_add_resources(nyaa-paper-qt "icons"
    PREFIX "/"
    BASE resources
    FILES resources/icon-128.png
)

install(TARGETS nyaa-paper-qt
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Domain tests: plain executable, no Qt types involved.
enable_testing()
add_executable(domain_test
    tests/domain_test.cpp
    src/domain/settings/Setting.cpp
    src/domain/settings/SettingFactory.cpp
    src/domain/settings/SettingList.cpp
    src/domain/Renderer.cpp
    src/domain/settings/Settings.cpp
    src/domain/options/GlobalOptions.cpp
)
target_include_directories(domain_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(domain_test PRIVATE -Wall -Wextra)
add_test(NAME domain_test COMMAND domain_test)

add_executable(projectjson_test tests/projectjson_test.cpp src/io/ProjectJson.cpp)
target_include_directories(projectjson_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(projectjson_test PRIVATE Qt6::Core)
add_test(NAME projectjson_test COMMAND projectjson_test)

add_executable(config_test tests/config_test.cpp src/io/ConfigStore.cpp src/io/WallpaperStore.cpp src/io/WallpaperConfigJson.cpp)
target_include_directories(config_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(config_test PRIVATE Qt6::Core)
add_test(NAME config_test COMMAND config_test)

add_executable(videoduration_test tests/videoduration_test.cpp src/util/VideoDuration.cpp)
target_include_directories(videoduration_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(videoduration_test PRIVATE Qt6::Core)
add_test(NAME videoduration_test COMMAND videoduration_test)

# PropertiesController with the mock engine: edits reach the command, and every
# monitor keeps its own config and GPU. Offscreen, test-mode config paths.
qt_add_executable(controller_test
    tests/controller_test.cpp
    src/bridge/PropertiesController.cpp
    src/bridge/ConfigBridge.cpp
    src/app/Autostart.cpp
    src/util/Gpu.cpp
    src/app/StartupRestore.cpp
    src/bridge/ConfigRenderer.cpp
    src/io/WallpaperConfigJson.cpp
    src/models/WallpaperModel.cpp
    src/bridge/EngineBridge.cpp
    src/bridge/PlaylistBridge.cpp
    src/bridge/ConfigRenderer.cpp
    src/models/WallpaperModel.cpp
    src/io/WallpaperConfigJson.cpp
    src/engine/DbusEngine.cpp
    src/engine/IEngine.hpp
    src/engine/MockEngine.hpp
    src/models/PropertyModel.cpp
    src/models/GlobalOptionsModel.cpp
    src/domain/settings/Setting.cpp
    src/domain/settings/Settings.cpp
    src/domain/settings/SettingFactory.cpp
    src/domain/settings/SettingList.cpp
    src/domain/options/GlobalOptions.cpp
    src/domain/Renderer.cpp
    src/io/ProjectJson.cpp
    src/io/ConfigStore.cpp
    src/io/WallpaperStore.cpp
    src/util/VideoDuration.cpp
    ${GUI_DBUS_SOURCES}
)
target_include_directories(controller_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(controller_test PRIVATE Qt6::Core Qt6::Gui Qt6::DBus)
add_test(NAME controller_test COMMAND controller_test)

# PlaylistBridge against a real daemon; needs a session bus with nyaa-paperd,
# so it is run by scripts/e2e.sh, not by ctest.
qt_add_executable(playlist_test
    tests/playlist_test.cpp
    src/bridge/PlaylistBridge.cpp
    src/bridge/ConfigBridge.cpp
    src/app/Autostart.cpp
    src/bridge/ConfigRenderer.cpp
    src/models/WallpaperModel.cpp
    src/domain/settings/Setting.cpp
    src/domain/settings/Settings.cpp
    src/domain/settings/SettingFactory.cpp
    src/domain/settings/SettingList.cpp
    src/domain/options/GlobalOptions.cpp
    src/domain/Renderer.cpp
    src/io/ProjectJson.cpp
    src/io/ConfigStore.cpp
    src/io/WallpaperConfigJson.cpp
    ${GUI_DBUS_SOURCES}
)
target_include_directories(playlist_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(playlist_test PRIVATE Qt6::Core Qt6::Gui Qt6::DBus)

add_executable(autostart_test tests/autostart_test.cpp src/app/Autostart.cpp)
target_include_directories(autostart_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(autostart_test PRIVATE Qt6::Core Qt6::DBus)
add_test(NAME autostart_test COMMAND autostart_test)
