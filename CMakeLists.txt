cmake_minimum_required(VERSION 3.16)
project(TC-tools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
endif()

add_executable(tctool
  src/main.cpp
  src/pages.cpp
  src/tools.cpp
  src/cli.cpp
  src/http.cpp
  src/json.cpp
  src/util.cpp
  src/app.cpp
  src/lang.cpp
)

target_compile_options(tctool PRIVATE -O2 -Wall)
target_compile_options(tctool PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
target_link_libraries(tctool PRIVATE winhttp shell32 user32 version advapi32)

set_target_properties(tctool PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist"
  OUTPUT_NAME "tctool"
)
