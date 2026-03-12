project(plasma-applet-babeleo)

set(babeleo_SRCS
    babeleo.cpp
    babelengine.cpp
)

# kcoreaddons_add_plugin creates a KDE plugin (shared library).
# INSTALL_NAMESPACE specifies the subdirectory below ${KDE_INSTALL_PLUGINDIR}.
# Plasma expects its applet plugins under plasma/applets/.
# The metadata.json file in the same directory is automatically embedded
# and used for plugin detection.
#
# Convention in Plasma 6: plugin filename = plugin ID (e.g. org.kde.plasma.babeleo.so).
# The CMake target is named "plasma_applet_babeleo" (avoid dots in target names),
# but OUTPUT_NAME sets the actual filename of the .so to the plugin ID.
kcoreaddons_add_plugin(plasma_applet_babeleo
    SOURCES ${babeleo_SRCS}
    INSTALL_NAMESPACE "plasma/applets"
)

set_target_properties(plasma_applet_babeleo PROPERTIES
    OUTPUT_NAME "org.kde.plasma.babeleo"
)

target_link_libraries(plasma_applet_babeleo
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
        KF6::I18n
        KF6::ConfigCore       # KConfigGroup
        KF6::GlobalAccel      # KGlobalAccel (global keyboard shortcuts)
        KF6::KIOCore          # KIO::StoredTransferJob
        KF6::KIOGui           # KIO::FavIconRequestJob
        KF6::WidgetsAddons    # KMessageBox
        KF6::WindowSystem     # KWaylandExtras, KWindowSystem (xdg-activation token)
        Plasma::Plasma        # Plasma::Applet base class
)

# plasma_install_package installiert das QML-Paket (UI, Config, etc.)
# nach ${KDE_INSTALL_DATADIR}/plasma/plasmoids/org.kde.plasma.babeleo/
# Plasma findet C++-Plugin und QML-Paket über die gemeinsame Plugin-Id.
plasma_install_package(package org.kde.plasma.babeleo)

add_subdirectory(icons)
