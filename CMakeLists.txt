cmake_minimum_required(VERSION 3.19)
project(monitor_music LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Multimedia sql)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

aux_source_directory(src SRC_LIST)

qt_add_executable(monitor_music
    WIN32 MACOSX_BUNDLE
    include/lyricwidget.h include/localsql.h include/mainwindow.h include/onlinerequest.h include/utils.h
    ${SRC_LIST}
    res.qrc

)

target_link_libraries(monitor_music
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Network
        Qt6::Multimedia
        Qt6::Sql
)

include(GNUInstallDirs)

install(TARGETS monitor_music
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET monitor_music
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
