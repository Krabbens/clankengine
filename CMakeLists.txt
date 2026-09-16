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

# Skeleton binary. Real modules (raylib/Box2D/Box3D) land in core/mX/ as agents claim them.
add_executable(clank src/main.cpp)
target_include_directories(clank PRIVATE core)

include(CTest)
add_test(NAME smoke-headless-dump COMMAND clank --headless --shot-after 60 --dump-scene ${CMAKE_BINARY_DIR}/scene.json --seed 42)
