cmake_minimum_required(VERSION 3.0)
project(PackHacks)

set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE GLVND)

file(GLOB sub_projects */CMakeLists.txt)
foreach(sub_project ${sub_projects})
    get_filename_component(directory ${sub_project} DIRECTORY)
    add_subdirectory(${directory})
endforeach()
