project(dependencies)

file(GLOB dependencies */CMakeLists.txt)
foreach(dependency ${dependencies})
    get_filename_component(directory ${dependency} DIRECTORY)
    add_subdirectory(${directory})
endforeach()
