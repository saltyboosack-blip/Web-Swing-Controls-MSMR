cmake_minimum_required(VERSION 3.24)

project(WebSwingControls VERSION 0.2.0 LANGUAGES C CXX)

if(NOT WIN32 OR NOT MSVC)
    message(FATAL_ERROR "Web Swing Controls requires Windows and MSVC.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Web Swing Controls supports only an x64 build (-A x64).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/minhook EXCLUDE_FROM_ALL)

add_library(WebSwingControls SHARED
    src/controls_dllmain.cpp
    src/ControlsOnlyRuntime.cpp
    src/GameBuild.cpp
    src/Logger.cpp
    src/ManualWebRuntime.cpp
    src/MemoryAccess.cpp
    src/MoverBinding.cpp
)

target_include_directories(WebSwingControls PRIVATE include src)
target_compile_definitions(WebSwingControls PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    TRUESWING_CONTROLS_ONLY
)
target_compile_options(WebSwingControls PRIVATE
    /W4
    /WX
    /permissive-
    /fp:precise
)
target_link_libraries(WebSwingControls PRIVATE minhook bcrypt)
set_target_properties(WebSwingControls PROPERTIES PREFIX "")

enable_testing()

function(add_policy_test target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE include src)
    target_compile_options(${target} PRIVATE
        /W4
        /WX
        /permissive-
        /fp:precise
    )
    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_policy_test(WebSwingControlsPolicyTests tests/ControlsOnlyPolicyTests.cpp)
add_policy_test(ControllerWebInputPolicyTests tests/ControllerWebInputPolicyTests.cpp)
add_policy_test(ControllerSourceRouteTests tests/ControllerSourceRouteTests.cpp)
add_policy_test(ManualWebPolicyTests tests/ManualWebPolicyTests.cpp)

