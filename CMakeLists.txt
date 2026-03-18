cmake_minimum_required(VERSION 3.16)

project(Fireworks)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# Source files
# ---------------------------------------------------------
set(SOURCES
    main.cpp
    src/camera.cpp
    src/particle.cpp
)

set(HEADERS
    include/camera.h
    include/particle.h
)

# Shader files (not compiled — added for IDE visibility in Solution Explorer)
set(SHADERS
    resource/particle.vert
    resource/particle.frag
    resource/ground.vert
    resource/ground.frag
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${SHADERS}
)

# ---------------------------------------------------------
# Include directories
# ---------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/resource
)

# ---------------------------------------------------------
# OpenGL
# ---------------------------------------------------------
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

# ---------------------------------------------------------
# External Libraries (GLEW, GLFW3, SOIL)
# Same library path as used in A1 / A2 / Tutorials.
# Change this if your library folder is in a different location.
# ---------------------------------------------------------
set(LIBRARY_PATH "C:/Users/sanam/OneDrive - Nipissing University/Desktop/3D_computer.graphics/libraries/libraries"
    CACHE PATH "Path to the provided libraries folder (include/, lib/, bin/)")

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)

    find_library(GLEW_LIBRARY NAMES glew32s glew32  HINTS ${LIBRARY_PATH}/lib REQUIRED)
    find_library(GLFW_LIBRARY NAMES glfw3           HINTS ${LIBRARY_PATH}/lib REQUIRED)
    find_library(SOIL_LIBRARY NAMES SOIL            HINTS ${LIBRARY_PATH}/lib REQUIRED)

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${LIBRARY_PATH}/include
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${GLEW_LIBRARY}
        ${GLFW_LIBRARY}
        ${SOIL_LIBRARY}
    )

    # Copy runtime DLLs next to the executable
    foreach(dll glew32.dll)
        if(EXISTS "${LIBRARY_PATH}/bin/${dll}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${LIBRARY_PATH}/bin/${dll}"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        elseif(EXISTS "${LIBRARY_PATH}/lib/${dll}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${LIBRARY_PATH}/lib/${dll}"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endif()
    endforeach()

    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "_d")

else()
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW glfw SOIL)
endif()
