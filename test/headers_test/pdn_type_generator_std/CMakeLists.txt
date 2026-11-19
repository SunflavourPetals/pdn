add_executable(test_pdn_type_generator_std test.cpp)
target_include_directories(test_pdn_type_generator_std PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_type_generator_std PRIVATE cxx_std_20)
# this TU pulls in pdn/spdn.h; with debug information (-g) the MinGW assembler
# rejects the resulting object as "file too big", so ask GNU as for COFF big objects
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(test_pdn_type_generator_std PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_type_generator_std COMMAND test_pdn_type_generator_std)
