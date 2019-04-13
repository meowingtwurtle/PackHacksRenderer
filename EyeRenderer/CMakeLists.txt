project(EyeRenderer)

file(GLOB_RECURSE headers include/*.hpp)
file(GLOB_RECURSE impl_headers include*.inl)
file(GLOB_RECURSE sources src/*.cpp)

add_library(EyeRenderer ${sources} ${headers} ${impl_headers})
set_target_properties(EyeRenderer PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(EyeRenderer PUBLIC include MeowingTwurtle::Engine::LowLevel MeowingTwurtle::Engine::Textures)

#target_link_libraries(EyeRenderer)
target_compile_options(EyeRenderer PRIVATE -Wall -Wextra)
