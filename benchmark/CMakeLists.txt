cmake_minimum_required(VERSION 3.20)

project(pdn_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR
		AND NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT DEFINED PDN_INCLUDE_DIR)
	set(PDN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../pdn/include"
		CACHE PATH "Directory that contains the pdn/ header directory")
endif()
if(NOT EXISTS "${PDN_INCLUDE_DIR}/pdn/spdn.h")
	message(FATAL_ERROR "PDN_INCLUDE_DIR does not contain pdn/spdn.h: ${PDN_INCLUDE_DIR}")
endif()

function(pdn_add_benchmark target)
	add_executable(${target} benchmark.cpp)
	target_include_directories(${target} PRIVATE "${PDN_INCLUDE_DIR}")
	target_compile_features(${target} PRIVATE cxx_std_20)

	# MinGW's assembler rejects the huge object produced by spdn.h when debug info (-g) is enabled.
	if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		target_compile_options(${target} PRIVATE "-Wa,-mbig-obj")
	endif()
endfunction()

pdn_add_benchmark(pdn_benchmark)

# Build the same code a second time to observe the cost of PDN_USER_OBJECT.
pdn_add_benchmark(pdn_benchmark_ordered)
target_compile_definitions(pdn_benchmark_ordered PRIVATE PDN_USER_OBJECT=::pdn::ordered_map)
