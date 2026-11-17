add_executable(test_pdn_type_to_type_code test.cpp)
target_include_directories(test_pdn_type_to_type_code PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_type_to_type_code PRIVATE cxx_std_20)
# this TU pulls in pdn/spdn.h; the MinGW assembler rejects the resulting object
# as "file too big" at -O0, so ask GNU as for COFF big objects
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(test_pdn_type_to_type_code PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_type_to_type_code COMMAND test_pdn_type_to_type_code)
