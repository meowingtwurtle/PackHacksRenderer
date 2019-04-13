project(EyeRenderer)

file(GLOB_RECURSE sources *.cpp)
file(GLOB_RECURSE headers *.hpp)

add_executable(EyeRenderer ${sources} ${headers})

target_link_libraries(EyeRenderer MeowingTwurtle::Engine::All GL glm stdc++fs)
