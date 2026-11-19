cmake_minimum_required(VERSION 3.21)
project(nyaa-paper VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 DBus)
qt_standard_project_setup(REQUIRES 6.5)

set(INTERFACES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces)
qt_add_dbus_interface(GUI_DBUS_SOURCES ${INTERFACES_DIR}/org.nyaa.Paper.Engine.xml   engine_proxy)
qt_add_dbus_interface(GUI_DBUS_SOURCES ${INTERFACES_DIR}/org.nyaa.Paper.Library.xml  library_proxy)
qt_add_dbus_interface(GUI_DBUS_SOURCES ${INTERFACES_DIR}/org.nyaa.Paper.Playlist.xml playlist_proxy)

qt_add_executable(nyaa-paper-qt
    src/main.cpp
    src/engine/IEngine.hpp
    src/engine/DbusEngine.hpp
    src/engine/DbusEngine.cpp
    src/engine/MockEngine.hpp
    src/bridge/EngineBridge.hpp
    src/bridge/EngineBridge.cpp
    src/models/WallpaperModel.hpp
    src/models/WallpaperModel.cpp
    src/models/MockLibrary.hpp
    src/models/MockLibrary.cpp
    src/ui/Theme.hpp
    src/ui/PreviewSettings.hpp
    src/ui/PreviewSettings.cpp
    ${GUI_DBUS_SOURCES}
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
        qml/components/Cell.qml
        qml/components/HintBar.qml
        qml/components/KeyCap.qml
        qml/components/MonitorPill.qml
        qml/components/Toast.qml
)

target_include_directories(nyaa-paper-qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(nyaa-paper-qt PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::DBus
)

install(TARGETS nyaa-paper-qt
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
