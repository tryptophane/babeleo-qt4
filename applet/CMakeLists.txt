project(babeleo)

add_subdirectory(icons)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   )

set(babeleo_SRCS babeleo.cpp 
                 babelengine.cpp)

kde4_add_ui_files(babeleo_SRCS
                  babeleoSettings.ui
                  babeleoOtherSettings.ui)

kde4_add_plugin(plasma_applet_babeleo ${babeleo_SRCS})
target_link_libraries(plasma_applet_babeleo
                      ${KDE4_PLASMA_LIBS}
                      ${KDE4_KDEUI_LIBS}
                      ${KDE4_KUTILS_LIBS}
                      kio
                     )

install(TARGETS plasma_applet_babeleo
        DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-babeleo.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})
