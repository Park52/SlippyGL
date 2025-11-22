cmake_minimum_required(VERSION 3.21)
project(SlippyGL LANGUAGES CXX)

# ---- C++ / Common ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS: OpenGL Core Profile 사용시 빌드 분기용 매크로(필요시 코드에서 #ifdef)
if(APPLE)
  add_definitions(-DSLIPPYGL_GL_CORE_PROFILE)
endif()

# ---- Dependencies via vcpkg ----
# vcpkg manifest 모드로 동작한다고 가정 (레포 루트의 vcpkg.json)
# Configure 시 -DCMAKE_TOOLCHAIN_FILE=~/.vcpkg/scripts/buildsystems/vcpkg.cmake 전달 필요
find_package(glfw3 CONFIG REQUIRED)
find_package(glad   CONFIG REQUIRED)
find_package(CURL   CONFIG REQUIRED)          # CURL::libcurl
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# ---- Vendored header-only (submodules / copied) ----
# 이 CMakeLists.txt는 SlippyGL/ 에 있으므로, include 경로는 ../external/ 로 올라감
add_library(glm::glm INTERFACE IMPORTED)
target_include_directories(glm::glm INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/../external/glm
)

add_library(stb::stb INTERFACE IMPORTED)
target_include_directories(stb::stb INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/../external/stb
)

# ---- Sources / Target ----
# 필요 시 세부 폴더를 추가하세요. (예: core/, net/, cache/, render/ ...)
file(GLOB_RECURSE SLIPPYGL_SRC
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/*.c
)

add_executable(SlippyGL ${SLIPPYGL_SRC})

target_include_directories(SlippyGL PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(SlippyGL PRIVATE
  glfw
  glad::glad
  CURL::libcurl
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  glm::glm
  stb::stb
)

# ---- Platform-specific OpenGL link ----
if (WIN32)
  target_link_libraries(SlippyGL PRIVATE opengl32)
elseif(APPLE)
  # macOS는 시스템 프레임워크 OpenGL 사용
  find_library(OPENGL_FRAMEWORK OpenGL)
  if(OPENGL_FRAMEWORK)
    target_link_libraries(SlippyGL PRIVATE ${OPENGL_FRAMEWORK})
  endif()
else()
  # Linux 등: 보통 glfw가 내부에서 처리하지만, 필요 시 명시
  find_package(OpenGL)
  if (OpenGL_FOUND)
    target_link_libraries(SlippyGL PRIVATE OpenGL::GL)
  endif()
endif()

# ---- Warnings / nice defaults ----
if (MSVC)
  target_compile_options(SlippyGL PRIVATE /W4 /permissive-)
else()
  target_compile_options(SlippyGL PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- (Optional) GL debug build hint ----
option(SLIPPYGL_ENABLE_GL_DEBUG "Enable OpenGL debug output in Debug builds" ON)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND SLIPPYGL_ENABLE_GL_DEBUG)
  target_compile_definitions(SlippyGL PRIVATE SLIPPYGL_GL_DEBUG)
endif()
