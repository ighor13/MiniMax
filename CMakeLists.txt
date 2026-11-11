cmake_minimum_required(VERSION 3.16)
project(MinimaxVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    nodeitem.cpp
    nodeitem.h
    gameutils.h
)

qt_add_executable(MinimaxVisualizer ${SOURCES})

if(ANDROID)
    set_target_properties(MinimaxVisualizer PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()

target_link_libraries(MinimaxVisualizer PRIVATE Qt6::Widgets)

qt_finalize_executable(MinimaxVisualizer)

if(WIN32)
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        set(DEPLOY_DIR "${CMAKE_BINARY_DIR}/deploy")
        
        set(DEPLOY_FLAG "--release")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            set(DEPLOY_FLAG "--debug")
        endif()

        add_custom_command(TARGET MinimaxVisualizer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:MinimaxVisualizer>"
                "${DEPLOY_DIR}/"
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                ${DEPLOY_FLAG}
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                "${DEPLOY_DIR}/MinimaxVisualizer.exe"
            COMMENT "Deploying to ${DEPLOY_DIR} with flag ${DEPLOY_FLAG}..."
        )
    endif()
endif()