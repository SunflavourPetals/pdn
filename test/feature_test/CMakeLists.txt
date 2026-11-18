cmake_minimum_required(VERSION 3.20)

project(pdn_feature_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PDN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../pdn/include"
	CACHE PATH "Directory that contains the pdn/ header directory")
if(NOT EXISTS "${PDN_INCLUDE_DIR}/pdn/spdn.h")
	message(FATAL_ERROR "PDN_INCLUDE_DIR does not contain pdn/spdn.h: ${PDN_INCLUDE_DIR}")
endif()

enable_testing()

add_executable(test_pdn_feature_test test.cpp)
target_include_directories(test_pdn_feature_test PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_feature_test PRIVATE cxx_std_20)

# MinGW 的汇编器在 -O0 下会拒绝 spdn.h 产生的超大对象
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	target_compile_options(test_pdn_feature_test PRIVATE "-Wa,-mbig-obj")
endif()

add_test(NAME pdn_feature_test
	COMMAND test_pdn_feature_test "${CMAKE_CURRENT_SOURCE_DIR}/..")
