cmake_minimum_required(VERSION 3.16)
project(MusicPlayer VERSION 1.0 LANGUAGES CXX)

# ---- C++ / Qt basics ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Helpful on Linux to run from build dir
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---- Qt 6 ----
find_package(Qt6 6.2 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Multimedia
        Svg
)

# ---- Sources (ROOT, not src/) ----
set(SRCS
        main.cpp
        MainWindow.cpp
        MainWindow.h
        resources.qrc
)

# Optional: organize in IDEs (CLion/VS)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SRCS})

# ---- Target ----
add_executable(MusicPlayer WIN32 MACOSX_BUNDLE ${SRCS})

target_link_libraries(MusicPlayer PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Multimedia
)

# Optional: nicer warnings
if(MSVC)
    target_compile_options(MusicPlayer PRIVATE /W4 /permissive-)
else()
    target_compile_options(MusicPlayer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Install (optional) ----
install(TARGETS MusicPlayer
        RUNTIME DESTINATION .
        BUNDLE  DESTINATION .
        LIBRARY DESTINATION .
)

target_link_libraries(MusicPlayer PRIVATE Qt6::Widgets Qt6::Multimedia Qt6::Svg)

# ---- Platform-specific deployment helpers ----
# Windows: windeployqt (copies platforms + multimedia/ffmpeg)
if(WIN32)
    target_link_libraries(MusicPlayer PRIVATE dwmapi)
    target_sources(MusicPlayer PRIVATE appicon.rc)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${Qt6_DIR}/../../../bin" "${CMAKE_PREFIX_PATH}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET MusicPlayer POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" --verbose 0
                --no-translations
                --multimedia
                "$<TARGET_FILE:MusicPlayer>"
                COMMENT "Running windeployqt to copy Qt runtime + multimedia plugins")
    else()
        message(WARNING "windeployqt not found; falling back to manual DLL copy (may miss some plugins).")
        set(DEBUG_SUFFIX "")
        if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
            set(DEBUG_SUFFIX "d")
        endif ()
        set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
                set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
            endif ()
        endif ()
        if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
            add_custom_command(TARGET MusicPlayer POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:MusicPlayer>/plugins/platforms/"
                    COMMAND ${CMAKE_COMMAND} -E copy
                    "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                    "$<TARGET_FILE_DIR:MusicPlayer>/plugins/platforms/")
        endif ()
        foreach (QT_LIB Core Gui Widgets Multimedia)
            add_custom_command(TARGET MusicPlayer POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                    "$<TARGET_FILE_DIR:MusicPlayer>")
        endforeach ()
        foreach (SUBDIR audio codecs multimedia)
            if (EXISTS "${QT_INSTALL_PATH}/plugins/${SUBDIR}")
                add_custom_command(TARGET MusicPlayer POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E make_directory
                        "$<TARGET_FILE_DIR:MusicPlayer>/plugins/${SUBDIR}/"
                        COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${QT_INSTALL_PATH}/plugins/${SUBDIR}"
                        "$<TARGET_FILE_DIR:MusicPlayer>/plugins/${SUBDIR}/")
            endif ()
        endforeach ()
    endif()
endif()

# macOS: bundle frameworks + plugins
if(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin" "${CMAKE_PREFIX_PATH}/bin")
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET MusicPlayer POST_BUILD
                COMMAND "${MACDEPLOYQT_EXECUTABLE}" "$<TARGET_BUNDLE_DIR:MusicPlayer>" -verbose=0 -no-translations -qmldir=. -extras -always-overwrite
                COMMENT "Running macdeployqt to bundle Qt frameworks + plugins")
    endif()
endif()

# Linux: RPATH for side-by-side libs
if(UNIX AND NOT APPLE)
    set_target_properties(MusicPlayer PROPERTIES
            BUILD_WITH_INSTALL_RPATH ON
            INSTALL_RPATH "$ORIGIN"
    )
endif()

message(STATUS "Hint: configure with -DCMAKE_PREFIX_PATH=<QtRoot>/lib/cmake (e.g. .../Qt/6.7.x/msvc2019_64/lib/cmake)")
