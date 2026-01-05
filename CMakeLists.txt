cmake_minimum_required(VERSION 3.25.1)

project(DiffusionCurveRenderer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(Qt6 COMPONENTS Core Widgets OpenGL OpenGLWidgets Gui Concurrent Xml REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs)

# Set up library directories
set(LIBS_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/Libs")
set(QT_IMGUI_DIR    "${LIBS_DIR}/qtimgui")
set(EIGEN_DIR       "${LIBS_DIR}/Eigen")

# Add qtimgui as a subdirectory (builds imgui and qt_imgui_widgets)
add_subdirectory(${QT_IMGUI_DIR})

# Include directories
set(INCLUDE_DIR
    "${EIGEN_DIR}/include"
    ${OpenCV_INCLUDE_DIRS}
)

file(GLOB_RECURSE SOURCES Source/*.cpp DiffusionCurveRenderer.qrc)

add_executable(DiffusionCurveRenderer ${SOURCES})

target_include_directories(DiffusionCurveRenderer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    ${INCLUDE_DIR}
)

target_link_libraries(DiffusionCurveRenderer
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::Concurrent
    Qt6::Xml
    ${OpenCV_LIBS}
    qt_imgui_widgets
)

# Copy resources to build directory
add_custom_command(TARGET DiffusionCurveRenderer
    POST_BUILD COMMAND ${CMAKE_COMMAND}
    -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources/"
    "$<TARGET_FILE_DIR:DiffusionCurveRenderer>/Resources/"
)
