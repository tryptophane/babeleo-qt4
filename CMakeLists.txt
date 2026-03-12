cmake_minimum_required(VERSION 3.22)
project(babeleo VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QT_DEFAULT_MAJOR_VERSION must be set BEFORE all find_package calls.
set(QT_DEFAULT_MAJOR_VERSION 6)

###
set(QT_MIN_VERSION "6.7.0")
set(KF6_MIN_VERSION "6.0.0")

# ECM (Extra CMake Modules) - the KDE-specific CMake toolkit
find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# IMPORTANT: Qt6 must be found BEFORE include(KDEInstallDirs)!
# KDEInstallDirs uses ecm_query_qt() to determine the plugin installation path
# (e.g. /usr/lib/qt6/plugins). This only works if Qt6::qmake is already known
# as a CMake target.
# Wrong order → KDEInstallDirs falls back to the Qt5 path /usr/lib/qt!
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Gui Widgets DBus)

# Include KDE standard CMake settings (AFTER find_package(Qt6)!)
include(KDEInstallDirs)       # Defines ${KDE_INSTALL_PLUGINDIR} etc.
include(KDECMakeSettings)     # Standard compile flags
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)       # Prints a summary of all found packages at the end

# KDE Frameworks 6 components
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons    # KPluginFactory, kcoreaddons_add_plugin
    I18n          # i18n(), KLocalizedString
    Config        # KConfigGroup, KConfig
    GlobalAccel   # KGlobalAccel (global keyboard shortcuts)
    KIO           # KIO::StoredTransferJob (network access)
    WidgetsAddons # KMessageBox, KUrlRequester
    XmlGui        # KKeySequenceWidget (keyboard shortcut widget)
)

# Plasma 6 Framework
find_package(Plasma REQUIRED)

# Optional runtime dependency: wl-clipboard (package: wl-clipboard)
# Required for Wayland Primary Selection (mouse selection without Ctrl+C).
# Without wl-clipboard, Babeleo falls back to Klipper (regular clipboard).
# Not REQUIRED - the build does not fail if wl-paste is missing.
find_program(WL_PASTE_EXECUTABLE wl-paste)
if(WL_PASTE_EXECUTABLE)
    message(STATUS "wl-paste found: ${WL_PASTE_EXECUTABLE}")
else()
    message(STATUS "wl-paste NOT found: On Wayland, mouse selection will not work.")
    message(STATUS "  → Install: sudo pacman -S wl-clipboard  (Arch/Manjaro)")
    message(STATUS "  → Install: sudo apt install wl-clipboard  (Debian/Ubuntu)")
endif()
add_feature_info("wl-clipboard (optional)" WL_PASTE_EXECUTABLE
    "Enables Wayland primary selection (mouse-selected text without Ctrl+C)")

add_subdirectory(applet)

# Shows a summary of all found/missing packages
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
