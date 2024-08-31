cmake_minimum_required(VERSION 3.16)

project(tt02 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6)
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
        main.cpp
        canvas.h        canvas.cpp
        chip.h          chip.cpp
        tt_grid.h       tt_grid.cpp
        readxml.h       readxml.cpp
        fetdata.h       fetdata.cpp
        readtimeconstraints.cpp  readtimeconstraints.h
        readspaceconstraints.h   readspaceconstraints.cpp
        lessontiles.h   lessontiles.cpp
        database.h      database.cpp
        showclass.h     showclass.cpp
        showteacher.h   showteacher.cpp
        showroom.h      showroom.cpp
        viewhandler.h   viewhandler.cpp
        basicconstraints.h  basicconstraints.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(tt02
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        with_roaring/roaring.c with_roaring/roaring.h
        with_roaring/roaring.hh
        with_roaring/use_roaring.cpp with_roaring/use_roaring.h
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET tt02 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(tt02 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(tt02
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(tt02 PRIVATE Qt6::Widgets)
target_link_libraries(tt02 PRIVATE Qt6::Sql)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.tt02)
endif()
set_target_properties(tt02 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS tt02
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(tt02)
endif()
