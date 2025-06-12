# ---------------------------------------------------------------------------
#  CMakeLists.txt  —  top-level build script for the "sameness" project
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)        # 3.16+ gives good IDE/xcodeproj
project(sameness VERSION 0.1 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
#  Locate Boost (header-only Asio + Boost.System for linking)
# ---------------------------------------------------------------------------
find_package(Boost REQUIRED COMPONENTS system)
if (NOT Boost_FOUND)
  message(FATAL_ERROR "Could not find Boost. Make sure BOOST_ROOT is set or Boost is installed.")
endif()

# ---------------------------------------------------------------------------
#  Locate OpenSSL (required by boost::asio::ssl)
# ---------------------------------------------------------------------------
find_package(OpenSSL REQUIRED)
if (NOT OpenSSL_FOUND)
  message(FATAL_ERROR "Could not find OpenSSL. Install it (e.g. brew install openssl@3) or set OPENSSL_ROOT_DIR.")
endif()

# Expose Boost headers to all targets
include_directories(${Boost_INCLUDE_DIRS})
# Expose OpenSSL headers to all targets (optional)
include_directories(${OpenSSL_INCLUDE_DIR})

# Define Windows-specific macros (only on Windows)
if (WIN32)
  add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

# ------------------------- Build settings -----------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_CXX_STANDARD 20)  # Windows needs C++20 for designated initializers
else()
    set(CMAKE_CXX_STANDARD 17)
endif()
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
    src/logger.c           # ⬅ remove this line if you don't use logger.c
)

add_library(sameness_core STATIC ${SAMENESS_CORE_SOURCES})

target_include_directories(sameness_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(sameness_core PRIVATE uiohook)

# ---------------------------------------------------------------------------
#  Client executable
# ---------------------------------------------------------------------------
add_executable(sameness_client src/network_client.cpp)

target_link_libraries(sameness_client
    PRIVATE
      sameness_core
      Boost::system        # for boost::system::error_code etc.
      OpenSSL::SSL         # for SSL functions (BIO_*, SSL_*)
      OpenSSL::Crypto      # for the underlying crypto symbols
)

# ---------------------------------------------------------------------------
#  Server executable
# ---------------------------------------------------------------------------
add_executable(sameness_server src/network_server.cpp)

target_link_libraries(sameness_server
    PRIVATE
      sameness_core
      Boost::system        # for boost::system::error_code etc.
      OpenSSL::SSL         # for SSL functions (BIO_*, SSL_*)
      OpenSSL::Crypto      # for the underlying crypto symbols
)

# ---------------------------------------------------------------------------
#  Optional install rules  (cmake --install build/)
# ---------------------------------------------------------------------------
install(TARGETS
        sameness_core
        sameness_client
        sameness_server
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

# ---------------------------------------------------------------------------
#  enable_testing()  # ← uncomment if/when you add tests under tests/
# ---------------------------------------------------------------------------
