cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------------------------
# Version: pdn/include/pdn/pdn_version.h is the single source of truth.
# ---------------------------------------------------------------------------
set(PDN_VERSION_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/pdn/include/pdn/pdn_version.h")
if(NOT EXISTS "${PDN_VERSION_HEADER}")
    message(FATAL_ERROR "pdn_version.h not found: ${PDN_VERSION_HEADER}")
endif()

file(READ "${PDN_VERSION_HEADER}" PDN_VERSION_HEADER_CONTENT)

foreach(_pdn_version_part MAJOR MINOR PATCH)
    if(NOT PDN_VERSION_HEADER_CONTENT MATCHES
            "#define[ \t]+PDN_VERSION_${_pdn_version_part}[ \t]+([0-9]+)")
        message(FATAL_ERROR
            "pdn_version.h: PDN_VERSION_${_pdn_version_part} is missing")
    endif()
    set(PDN_VERSION_${_pdn_version_part} "${CMAKE_MATCH_1}")
endforeach()

if(NOT PDN_VERSION_HEADER_CONTENT MATCHES
        "#define[ \t]+PDN_VERSION_STRING[ \t]+\"([^\"]+)\"")
    message(FATAL_ERROR "pdn_version.h: PDN_VERSION_STRING is missing")
endif()
set(PDN_VERSION_STRING "${CMAKE_MATCH_1}")

if(NOT PDN_VERSION_STRING STREQUAL
        "${PDN_VERSION_MAJOR}.${PDN_VERSION_MINOR}.${PDN_VERSION_PATCH}")
    message(FATAL_ERROR
        "pdn_version.h is inconsistent: PDN_VERSION_STRING is "
        "\"${PDN_VERSION_STRING}\", but the numeric macros say "
        "\"${PDN_VERSION_MAJOR}.${PDN_VERSION_MINOR}.${PDN_VERSION_PATCH}\"")
endif()

project(pdn VERSION "${PDN_VERSION_STRING}" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(PDN_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(PDN_IS_TOP_LEVEL ON)
endif()

option(PDN_BUILD_DEMO
    "Build the hello-world demo from pdn/main.cpp" ${PDN_IS_TOP_LEVEL})
option(PDN_BUILD_TESTS
    "Build the header and feature tests" ${PDN_IS_TOP_LEVEL})
option(PDN_BUILD_DOC_EXAMPLES
    "Build the examples under docs/guide-source" OFF)
option(PDN_BUILD_BENCHMARKS
    "Build the benchmark executables" OFF)
option(PDN_INSTALL
    "Generate install rules for the pdn package" ${PDN_IS_TOP_LEVEL})
option(PDN_INSTALL_DOCS
    "Install the documentation" ${PDN_IS_TOP_LEVEL})
option(PDN_SKIP_TOOLCHAIN_CHECK
    "Skip the C++20 standard-library feature check" OFF)

# ---------------------------------------------------------------------------
# Toolchain check: the library itself needs std::format and floating-point
# std::from_chars.  Users can provide replacements through PDN_USER_FORMAT /
# PDN_USER_FROM_CHARS_FLOAT; in that case configure with
# -DPDN_SKIP_TOOLCHAIN_CHECK=ON.
# ---------------------------------------------------------------------------
if(PDN_IS_TOP_LEVEL AND NOT PDN_SKIP_TOOLCHAIN_CHECK)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles([=[
        #include <charconv>
        #include <format>
        #include <string>

        int main()
        {
            const std::string text = std::format("{}", 1.5);
            double value = 0.0;
            const char* first = "1.5";
            const char* last = first + 3;
            std::from_chars(first, last, value);
            std::from_chars(first, last, value, std::chars_format::hex);
            return text.empty() ? 1 : 0;
        }
    ]=] PDN_HAS_REQUIRED_CXX20_LIBRARY)

    if(NOT PDN_HAS_REQUIRED_CXX20_LIBRARY)
        message(FATAL_ERROR
            "pdn requires C++20 std::format and floating-point "
            "std::from_chars. Use GCC >= 13, Clang >= 17 with a C++20 "
            "standard library, or MSVC >= 19.29. If you provide replacements "
            "through PDN_USER_FORMAT / PDN_USER_FROM_CHARS_FLOAT, reconfigure "
            "with -DPDN_SKIP_TOOLCHAIN_CHECK=ON.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Header-only library target
# ---------------------------------------------------------------------------
add_library(pdn INTERFACE)
add_library(pdn::pdn ALIAS pdn)

target_compile_features(pdn INTERFACE cxx_std_20)
target_include_directories(pdn INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pdn/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# MinGW's assembler needs COFF big objects for translation units that include
# the umbrella header; the trigger is debug information (-g), not an -O level.
# This must be an INTERFACE option so consumers of the installed package get it
# as well; the conditions are evaluated on the consumer's toolchain.
target_compile_options(pdn INTERFACE
    "$<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:GNU>>:-Wa,-mbig-obj>")

# ---------------------------------------------------------------------------
# Demo
# ---------------------------------------------------------------------------
if(PDN_BUILD_DEMO)
    add_executable(pdn_demo pdn/main.cpp)
    target_link_libraries(pdn_demo PRIVATE pdn::pdn)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(PDN_BUILD_TESTS)
    enable_testing()

    # The test subprojects use this cache variable to find the headers.
    set(PDN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pdn/include"
        CACHE PATH "PDN include root (parent of the directory holding pdn/spdn.h)")

    add_subdirectory(test/headers_test)
    add_subdirectory(test/feature_test)

    if(TARGET pdn_demo)
        add_test(NAME pdn_demo_smoke COMMAND pdn_demo)
        set_tests_properties(pdn_demo_smoke PROPERTIES
            PASS_REGULAR_EXPRESSION "Hello world!")
    endif()
endif()

# ---------------------------------------------------------------------------
# Documentation examples
# ---------------------------------------------------------------------------
if(PDN_BUILD_DOC_EXAMPLES)
    enable_testing()
    add_subdirectory(docs/guide-source)
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
if(PDN_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()

# ---------------------------------------------------------------------------
# Install / find_package / pkg-config
# ---------------------------------------------------------------------------
if(PDN_INSTALL)
    install(TARGETS pdn EXPORT pdnTargets)

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pdn/include/pdn"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            FILES_MATCHING PATTERN "*.h")

    install(FILES LICENSE README.md
            DESTINATION "${CMAKE_INSTALL_DOCDIR}")

    if(PDN_INSTALL_DOCS)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/"
                DESTINATION "${CMAKE_INSTALL_DOCDIR}"
                FILES_MATCHING
                    PATTERN "*.md"
                    PATTERN "*.txt"
                    PATTERN "*.spdn"
                    PATTERN "*.cpp"
                    PATTERN "*.h"
                    PATTERN "CMakeLists.txt")
    endif()

    set(PDN_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/pdn"
        CACHE STRING "Directory for the pdn CMake package files")

    install(EXPORT pdnTargets
            FILE pdnTargets.cmake
            NAMESPACE pdn::
            DESTINATION "${PDN_INSTALL_CMAKEDIR}")

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pdnConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/pdnConfig.cmake"
        INSTALL_DESTINATION "${PDN_INSTALL_CMAKEDIR}")

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/pdnConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMinorVersion
        ARCH_INDEPENDENT)

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/pdnConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/pdnConfigVersion.cmake"
            DESTINATION "${PDN_INSTALL_CMAKEDIR}")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pdn.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/pdn.pc"
        @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pdn.pc"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()
