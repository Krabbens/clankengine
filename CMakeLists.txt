cmake_minimum_required(VERSION 3.28)
project(clankengine LANGUAGES C CXX)
if(APPLE)
  # WHY: Box3D enables OBJC for its Apple files; a new language can only be
  # enabled from top-level scope, not from inside FetchContent.
  enable_language(OBJC)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# WHY interface target, not add_compile_options: global flags would leak into
# FetchContent builds (raylib's own warnings would become errors there).
# Our code opts in per target; third-party builds stay untouched.
add_library(clank_warnings INTERFACE)
if(MSVC)
  target_compile_options(clank_warnings INTERFACE /W4 /WX)
else()
  target_compile_options(clank_warnings INTERFACE -Wall -Wextra -Werror)
endif()

# Third-party backends (network fetch, pinned in .github/ci-config.yml).
# Public headers stay stdlib-only; only impl/ TUs link these targets.
# Impl TUs are listed explicitly (no GLOB) so renames fail loud.
include(cmake/third_party.cmake)

add_library(m0_foundation STATIC core/m0-foundation/impl/foundation.cpp)
target_link_libraries(m0_foundation PUBLIC clank_warnings)
target_include_directories(m0_foundation PUBLIC core/m0-foundation/public)

add_library(m1_loop STATIC core/m1-loop/impl/loop.cpp)
target_link_libraries(m1_loop PUBLIC clank_warnings)
# WHY PUBLIC: static lib keeps raylib refs; final exes must see them to link.
target_link_libraries(m1_loop PUBLIC raylib)
target_include_directories(m1_loop PUBLIC core/m1-loop/public)

add_library(m2_render STATIC core/m2-render/impl/render.cpp)
target_link_libraries(m2_render PUBLIC clank_warnings)
# WHY PUBLIC: same static-link propagation as m1.
target_link_libraries(m2_render PUBLIC raylib)
target_include_directories(m2_render PUBLIC core/m2-render/public)

add_library(m3_physics2d STATIC core/m3-physics2d/impl/physics2d.cpp)
target_link_libraries(m3_physics2d PUBLIC clank_warnings)
target_link_libraries(m3_physics2d PRIVATE box2d)
target_include_directories(m3_physics2d PUBLIC core/m3-physics2d/public)

add_library(m4_physics3d STATIC core/m4-physics3d/impl/physics3d.cpp)
target_link_libraries(m4_physics3d PUBLIC clank_warnings)
target_link_libraries(m4_physics3d PRIVATE box3d)
target_include_directories(m4_physics3d PUBLIC core/m4-physics3d/public)

add_library(m5_scene STATIC core/m5-scene/impl/scene.cpp)
target_link_libraries(m5_scene PUBLIC clank_warnings)
target_include_directories(m5_scene PUBLIC core/m5-scene/public)

add_library(m6_input STATIC core/m6-input/impl/input.cpp)
target_link_libraries(m6_input PUBLIC clank_warnings)
target_include_directories(m6_input PUBLIC core/m6-input/public)

# clank app: headless-first game loop on m0/m1/m2/m5/m6.
add_executable(clank src/main.cpp)
target_link_libraries(clank PRIVATE m0_foundation m1_loop m2_render m5_scene m6_input clank_warnings)

# m8 sample: headless scene dump/reload demo.
add_executable(minimal examples/minimal.cpp)
target_link_libraries(minimal PRIVATE m1_loop m5_scene clank_warnings)

include(CTest)

foreach(sample pegboard marble_tray zelda)
  add_executable(${sample} examples/${sample}.cpp)
  target_link_libraries(${sample} PRIVATE m0_foundation m1_loop m2_render
    m5_scene m6_input clank_warnings)
endforeach()
target_link_libraries(pegboard PRIVATE m3_physics2d)
target_link_libraries(marble_tray PRIVATE m4_physics3d)

if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_executable(showcase-pixels test/showcase_pixels.cpp)
  target_link_libraries(showcase-pixels PRIVATE raylib clank_warnings)
  add_test(NAME showcase-retina-golden COMMAND showcase-pixels
    ${CMAKE_SOURCE_DIR}/test/golden/pegboard_linux_frame120.png
    ${CMAKE_SOURCE_DIR}/test/golden/pegboard_frame120.png)
  foreach(sample pegboard marble_tray)
    add_test(NAME ${sample}-replay COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_SOURCE_DIR}/test/showcase_test.py $<TARGET_FILE:${sample}>
      ${CMAKE_SOURCE_DIR}/examples/replays/drop.clk)
    if(sample STREQUAL "pegboard")
      set(frame 120)
    else()
      set(frame 60)
    endif()
    if(NOT WIN32)
      add_test(NAME ${sample}-golden COMMAND bash ${CMAKE_SOURCE_DIR}/test/showcase_golden.sh
        $<TARGET_FILE:${sample}> $<TARGET_FILE:showcase-pixels>
        ${CMAKE_SOURCE_DIR}/test/golden/${sample}_frame${frame}.png
        ${CMAKE_SOURCE_DIR}/examples/replays/drop.clk ${frame})
    endif()
  endforeach()
  add_test(NAME zelda-smoke COMMAND zelda --headless --shot-after 60 --dump-scene
    ${CMAKE_BINARY_DIR}/zelda_scene.json --seed 42)
  add_test(NAME zelda-win COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/test/zelda_win_test.py $<TARGET_FILE:zelda>
    ${CMAKE_SOURCE_DIR}/examples/replays/zelda_win.clk)
endif()

function(clank_selftest name lib src inc)
  add_executable(${name} ${src})
  target_include_directories(${name} PRIVATE ${inc})
  target_link_libraries(${name} PRIVATE ${lib} clank_warnings)
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
add_test(NAME clank-replay COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/test/clank_replay_test.py $<TARGET_FILE:clank>
  ${CMAKE_SOURCE_DIR}/examples/replays/drop.clk)
add_test(NAME claim-worktree-isolation COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_SOURCE_DIR}/test/claim_worktree_test.py ${CMAKE_SOURCE_DIR}/tools/clank-claim)
add_test(NAME minimal-sample COMMAND minimal)

# WHY shell compare, unix-only: CTest cannot diff stdout; CI matrix is ubuntu-only.
# Golden is byte-exact: minimal is fixed-seed IEEE add/mul only, no libm calls.
if(NOT WIN32)
  add_test(NAME golden-minimal-scene
    COMMAND sh -c "$<TARGET_FILE:minimal> > ${CMAKE_BINARY_DIR}/minimal_out.json && ${CMAKE_COMMAND} -E compare_files ${CMAKE_BINARY_DIR}/minimal_out.json ${CMAKE_SOURCE_DIR}/test/golden/minimal_scene.json")
  # WHY script, not inline: windowed run needs xvfb when present, skip otherwise.
  add_test(NAME windowed-golden
    COMMAND sh ${CMAKE_SOURCE_DIR}/test/windowed_golden.sh $<TARGET_FILE:clank> ${CMAKE_SOURCE_DIR}/test/golden/windowed_frame60.png)
endif()
