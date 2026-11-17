add_executable(test_pdn_parse test.cpp)
target_include_directories(test_pdn_parse PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_parse PRIVATE cxx_std_20)
# this TU pulls in the whole parse stack; the MinGW assembler rejects the
# resulting object as "file too big", so ask GNU as for COFF big objects
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(test_pdn_parse PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_parse COMMAND test_pdn_parse)
