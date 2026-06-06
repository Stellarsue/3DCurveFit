cmake_minimum_required(VERSION 3.10)

project(PolynomialCurveFitOSG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(OpenSceneGraph REQUIRED COMPONENTS osg osgDB osgGA osgViewer)

find_package(Qt6 QUIET COMPONENTS Widgets OpenGLWidgets)
if(Qt6_FOUND)
    set(QT_LIBRARIES Qt6::Widgets Qt6::OpenGLWidgets)
else()
    find_package(Qt5 REQUIRED COMPONENTS Widgets)
    set(QT_LIBRARIES Qt5::Widgets)
endif()

function(remove_agl_framework_from_target target_name)
    if(NOT APPLE OR NOT TARGET ${target_name})
        return()
    endif()

    foreach(prop INTERFACE_LINK_LIBRARIES INTERFACE_LINK_OPTIONS)
        get_target_property(values ${target_name} ${prop})
        if(values)
            set(cleaned_values)
            list(LENGTH values value_count)
            set(index 0)
            while(index LESS value_count)
                list(GET values ${index} value)

                if(value STREQUAL "-framework")
                    math(EXPR next_index "${index} + 1")
                    if(next_index LESS value_count)
                        list(GET values ${next_index} framework_name)
                        if(framework_name STREQUAL "AGL")
                            math(EXPR index "${index} + 2")
                            continue()
                        endif()
                    endif()
                endif()

                if(value STREQUAL "AGL" OR value MATCHES "AGL\\.framework$")
                    math(EXPR index "${index} + 1")
                    continue()
                endif()

                list(APPEND cleaned_values "${value}")
                math(EXPR index "${index} + 1")
            endwhile()

            set_target_properties(${target_name} PROPERTIES ${prop} "${cleaned_values}")
        endif()
    endforeach()
endfunction()

remove_agl_framework_from_target(WrapOpenGL::WrapOpenGL)
remove_agl_framework_from_target(Qt6::Gui)
remove_agl_framework_from_target(Qt6::OpenGL)
remove_agl_framework_from_target(Qt6::OpenGLWidgets)
remove_agl_framework_from_target(Qt5::Gui)

add_executable(PolynomialCurveFitOSG
    main.cpp
    src/core/GeometryTypes.cpp
    src/core/PointCloudPipeline.cpp
    src/fitting/PolynomialCurveFitter.cpp
    src/fitting/BSplineCurveFitter.cpp
    src/fitting/BezierCurveFitter.cpp
    src/visualization/OsgScene.cpp
    src/ui/OsgWidget.cpp
    src/ui/MainWindow.cpp
)

target_include_directories(PolynomialCurveFitOSG PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENSCENEGRAPH_INCLUDE_DIRS}
)

target_link_libraries(PolynomialCurveFitOSG PRIVATE
    ${OPENSCENEGRAPH_LIBRARIES}
    ${QT_LIBRARIES}
)

if(APPLE)
    target_compile_definitions(PolynomialCurveFitOSG PRIVATE GL_SILENCE_DEPRECATION)
endif()
