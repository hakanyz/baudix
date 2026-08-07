cmake_minimum_required(VERSION 3.16)

project(baudix VERSION 1.3.1 LANGUAGES CXX)

add_compile_definitions(BAUDIX_VERSION_STR="${PROJECT_VERSION}")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets SerialPort Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets SerialPort Network)

set(PROJECT_SOURCES
        src/main.cpp
        src/UI/mainwindow.cpp
        src/UI/mainwindow.h
        src/UI/mainwindow.ui
        src/UI/ConnectionWidget.cpp
        src/UI/ConnectionWidget.h
        src/UI/TerminalWidget.cpp
        src/UI/TerminalWidget.h
        src/UI/TerminalModel.cpp
        src/UI/TerminalModel.h
        src/UI/BadgeDelegate.cpp
        src/UI/BadgeDelegate.h
        src/UI/DataColumnDelegate.cpp
        src/UI/DataColumnDelegate.h
        src/UI/SendWidget.cpp
        src/UI/SendWidget.h
        src/UI/LoggingWidget.cpp
        src/UI/LoggingWidget.h
        src/UI/MacroWidget.cpp
        src/UI/MacroWidget.h
        src/Core/Updater.cpp
        src/Core/Updater.h
        src/Communication/ISerialTransport.h
        src/Communication/SerialPortController.cpp
        src/Communication/SerialPortController.h
        resources/resources.qrc
)

if(WIN32)
    list(APPEND PROJECT_SOURCES resources/app.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(baudix
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET baudix APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(baudix SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(baudix
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(baudix PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::SerialPort Qt${QT_VERSION_MAJOR}::Network)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.baudix)
endif()
set_target_properties(baudix PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS baudix
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    install(FILES resources/baudix.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(FILES resources/baudix_icon.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME baudix.svg
    )
    install(FILES resources/baudix_icon.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
        RENAME baudix.svg
    )
    install(FILES resources/99-baudix-udev.rules
        DESTINATION /lib/udev/rules.d
    )
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(baudix)
endif()

# --- CPack Configuration for Linux (.deb) ---
if(UNIX AND NOT APPLE AND NOT ANDROID)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "baudix")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_CONTACT "hakanyz")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Professional Serial Terminal for Embedded Systems")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "hakanyz")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6gui6, libqt6widgets6, libqt6serialport6, libqt6network6")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinst")
    include(CPack)
endif()
