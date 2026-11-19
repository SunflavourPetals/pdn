cmake_minimum_required(VERSION 3.20)

project(pdn_guide_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer an installed package when this directory is configured on its own;
# when included from the repository root the pdn::pdn target already exists.
if(NOT TARGET pdn::pdn)
    find_package(pdn CONFIG QUIET)
endif()

if(NOT TARGET pdn::pdn)
    add_library(pdn_guide_headers INTERFACE)
    add_library(pdn::pdn ALIAS pdn_guide_headers)
    target_include_directories(pdn_guide_headers INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../pdn/include")
    target_compile_features(pdn_guide_headers INTERFACE cxx_std_20)
    target_compile_options(pdn_guide_headers INTERFACE
        "$<$<AND:$<PLATFORM_ID:Windows>,$<CXX_COMPILER_ID:GNU>>:-Wa,-mbig-obj>")
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
endif()

set(PDN_GUIDE_EXAMPLES
    as-example
    get-example
    get-if-example
    get-opt-example
    hello-spdn
    learn-parse
    ordered-map
    pdn-parser-impl
    print-err-msg-to-file
    query-by-key
    query-on-list
    type-test-example
    ud-at-example)

foreach(_name IN LISTS PDN_GUIDE_EXAMPLES)
    string(REPLACE "-" "_" _target "${_name}")

    add_executable(${_target} "${_name}.cpp")
    target_link_libraries(${_target} PRIVATE pdn::pdn)
    set_target_properties(${_target} PROPERTIES OUTPUT_NAME "${_name}")

    add_test(NAME "guide_${_target}"
             COMMAND ${_target}
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endforeach()

# The examples read these files from their working directory.  Copy them so
# the build directory is self-contained and the tracked files stay untouched.
file(COPY
    "${CMAKE_CURRENT_SOURCE_DIR}/hello.spdn"
    "${CMAKE_CURRENT_SOURCE_DIR}/example.spdn"
    "${CMAKE_CURRENT_SOURCE_DIR}/print-err-msg-to-file.txt"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
