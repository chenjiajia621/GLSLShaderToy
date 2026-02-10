cmake_minimum_required(VERSION 3.16)

project(ShaderEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick ShaderTools Gui)

qt_standard_project_setup(REQUIRES 6.8)

file(GLOB APP_OTHERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    CMAKE_CONFIGURE_DEPENDS
    "assets/others/*.*"
)

set(TARGET_NAME shaderToy)

set(CORE_FILES
    HighlighterShader.h HighlighterShader.cpp
    myrhiitem.h myrhiitem.cpp
    rhipingpongitem.h rhipingpongitem.cpp
    StructModel.h
    FileHelper.h
)
list(TRANSFORM CORE_FILES PREPEND "src/core/")

set(COMPOENTS_FILES
    CShaderView.qml
)

list(TRANSFORM COMPOENTS_FILES PREPEND "src/components/")

qt_add_executable(${TARGET_NAME}
   	main.cpp
	app.rc
)

qt_add_qml_module(${TARGET_NAME}
    URI "MyRhi"
    VERSION 1.0
    QML_FILES
    Main.qml
    ${COMPOENTS_FILES}
    SOURCES
    ${CORE_FILES}
    RESOURCES
    ${APP_OTHERS}
)

qt_add_shaders(${TARGET_NAME} "my_shaders"
    PRECOMPILE
    OPTIMIZE NONE
    GLSL "300 es, 330, 450"
    PREFIX
    /myfile
    BASE
    ./shaders
    FILES
        ./shaders/common.vert
)

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
    Qt6::Quick
    Qt6::ShaderTools
    Qt6::Gui
    Qt6::GuiPrivate
    Qt::ShaderToolsPrivate
)
