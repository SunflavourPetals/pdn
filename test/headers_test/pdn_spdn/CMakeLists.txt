add_executable(test_pdn_spdn test.cpp)
target_include_directories(test_pdn_spdn PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_spdn PRIVATE cxx_std_20)
# the umbrella pulls in the whole library; the MinGW assembler rejects the
# resulting object as "file too big", so ask GNU as for COFF big objects
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(test_pdn_spdn PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_spdn COMMAND test_pdn_spdn)
