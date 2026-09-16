cmake_minimum_required(VERSION 3.28)
project(clankengine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Werror)
endif()

# One static lib per core module. Impl TUs are listed explicitly (no GLOB)
# so renames fail loud. Third-party backends (raylib/Box2D/Box3D) attach
# inside impl/ in wave 2; public headers stay stdlib-only.

add_library(m0_foundation STATIC core/m0-foundation/impl/foundation.cpp)
target_include_directories(m0_foundation PUBLIC core/m0-foundation/public)

add_library(m1_loop STATIC core/m1-loop/impl/loop.cpp)
target_include_directories(m1_loop PUBLIC core/m1-loop/public)

add_library(m2_render STATIC core/m2-render/impl/render.cpp)
target_include_directories(m2_render PUBLIC core/m2-render/public)

add_library(m3_physics2d STATIC core/m3-physics2d/impl/physics2d.cpp)
target_include_directories(m3_physics2d PUBLIC core/m3-physics2d/public)

add_library(m4_physics3d STATIC core/m4-physics3d/impl/physics3d.cpp)
target_include_directories(m4_physics3d PUBLIC core/m4-physics3d/public)

add_library(m5_scene STATIC core/m5-scene/impl/scene.cpp)
target_include_directories(m5_scene PUBLIC core/m5-scene/public)

add_library(m6_input STATIC core/m6-input/impl/input.cpp)
target_include_directories(m6_input PUBLIC core/m6-input/public)

# Skeleton binary. Adopts m0 Flags in wave 2; still standalone today.
add_executable(clank src/main.cpp)
target_include_directories(clank PRIVATE core)

# m8 sample: headless scene dump/reload demo.
add_executable(minimal examples/minimal.cpp)
target_link_libraries(minimal PRIVATE m5_scene)

include(CTest)

function(clank_selftest name lib src inc)
  add_executable(${name} ${src})
  target_include_directories(${name} PRIVATE ${inc})
  target_link_libraries(${name} PRIVATE ${lib})
  add_test(NAME ${name} COMMAND ${name})
endfunction()

clank_selftest(m0-selftest m0_foundation core/m0-foundation/selftest.cpp core/m0-foundation/public)
clank_selftest(m1-selftest m1_loop core/m1-loop/selftest.cpp core/m1-loop/public)
clank_selftest(m2-selftest m2_render core/m2-render/selftest.cpp core/m2-render/public)
clank_selftest(m3-selftest m3_physics2d core/m3-physics2d/selftest.cpp core/m3-physics2d/public)
clank_selftest(m4-selftest m4_physics3d core/m4-physics3d/selftest.cpp core/m4-physics3d/public)
clank_selftest(m5-selftest m5_scene core/m5-scene/selftest.cpp core/m5-scene/public)
clank_selftest(m6-selftest m6_input core/m6-input/selftest.cpp core/m6-input/public)

add_test(NAME smoke-headless-dump COMMAND clank --headless --shot-after 60 --dump-scene ${CMAKE_BINARY_DIR}/scene.json --seed 42)
add_test(NAME minimal-sample COMMAND minimal)
