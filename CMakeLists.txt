# ---------------------------------------------------------------------------
#  CMakeLists.txt  —  top-level build script for the “sameness” project
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)        # 3.16+ gives good IDE/xcodeproj
project(sameness VERSION 0.1 LANGUAGES C CXX)

# ------------------------- Build settings -----------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
#  Third-party dependency: libuiohook
#  (You added this as a git sub-module under third_party/libuiohook)
# ---------------------------------------------------------------------------
add_subdirectory(third_party/libuiohook)

# ---------------------------------------------------------------------------
#  Core library (no main()) — shared by client + server
# ---------------------------------------------------------------------------
set(SAMENESS_CORE_SOURCES
    src/EventPacket.cpp
    src/EventState.cpp
    src/Injectors.cpp
    src/logger.c              # ⬅ remove this line if you don’t use logger.c
)

add_library(sameness_core STATIC ${SAMENESS_CORE_SOURCES})

target_include_directories(sameness_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include      # public headers, if any
)
target_compile_definitions(sameness_core PRIVATE _WIN32_WINNT=0x0A00)
target_link_libraries(sameness_core PRIVATE uiohook)

# ---------------------------------------------------------------------------
#  Client executable
# ---------------------------------------------------------------------------
add_executable(sameness_client src/network_client.cpp)
target_compile_definitions(sameness_client PRIVATE _WIN32_WINNT=0x0A00)
target_link_libraries(sameness_client PRIVATE sameness_core)

# ---------------------------------------------------------------------------
#  Server executable
# ---------------------------------------------------------------------------
add_executable(sameness_server src/network_server.cpp)
target_compile_definitions(sameness_server PRIVATE _WIN32_WINNT=0x0A00)
target_link_libraries(sameness_server PRIVATE sameness_core)

# ---------------------------------------------------------------------------
#  Optional install rules  (cmake --install build/)
# ---------------------------------------------------------------------------
install(TARGETS
        sameness_core          # static lib
        sameness_client        # exe
        sameness_server        # exe
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# ---------------------------------------------------------------------------
#  enable_testing()  # ← uncomment if/when you add tests under tests/
# ---------------------------------------------------------------------------
