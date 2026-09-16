# Third-party deps via FetchContent. Versions MUST match .github/ci-config.yml.
# Pre-seeded *-OFF cache vars are harmless if a project does not declare them;
# they win when it does (keeps samples/tests out of our build).
include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)

set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)

set(BOX3D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX3D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX3D_BENCHMARKS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
)
FetchContent_Declare(
  box2d
  URL https://github.com/erincatto/box2d/archive/refs/tags/v3.1.1.tar.gz
)
FetchContent_Declare(
  box3d
  URL https://github.com/erincatto/box3d/archive/8441b4a06d6d09dcfb0b0f704df4d847d1437b92.tar.gz
)

FetchContent_MakeAvailable(raylib box2d box3d)
