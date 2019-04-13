project(EyeRenderer)

file(GLOB_RECURSE sources *.cpp)
file(GLOB_RECURSE headers *.hpp)

add_executable(EyeRenderer ${sources} ${headers})

target_link_libraries(EyeRenderer MeowingTwurtle::Engine::All GL glm stdc++fs)
target_compile_options(
        EyeRenderer
        PRIVATE
        -Wall
        -Wextra
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        #        -Wconversion
        #                -Wsign-conversion
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wnull-dereference
        -Wuseless-cast
        #                -Wdouble-promotion
        -Wformat=2
        -Wlifetime
        -Weffc++
        -Wnarrowing
)

set(UseOptimizer FALSE)
set(UseLTO TRUE)
set(UseSanitizers FALSE)

if (UseOptimizer)
    target_compile_options(EyeRenderer PRIVATE -O3)

    if (UseLTO)
        target_compile_options(EyeRenderer PRIVATE -flto)
        target_link_options(EyeRenderer PRIVATE -flto)
    endif ()
else ()
    target_compile_options(EyeRenderer PRIVATE -O0)
endif ()

if (UseSanitizers)
    set(SanitizerChecks address,undefined,integer)
    set(SanitizerArgs -fsanitize=${SanitizerChecks} -fno-sanitize-recover=${SanitizerChecks})

    target_compile_options(EyeRenderer PRIVATE ${SanitizerArgs})
    target_link_options(EyeRenderer PRIVATE ${SanitizerArgs})
endif ()
