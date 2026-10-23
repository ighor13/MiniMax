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

# Создаем цель (для Android и десктопа)
qt_add_executable(MinimaxVisualizer ${SOURCES})

# Настройки специфичные для Android
if(ANDROID)
    # Указываем путь к папке с манифестом (один раз для всех архитектур)
    set_target_properties(MinimaxVisualizer PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()

target_link_libraries(MinimaxVisualizer PRIVATE Qt6::Widgets)

# Обязательная финализация для Qt 6
qt_finalize_executable(MinimaxVisualizer)