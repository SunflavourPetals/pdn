add_executable(test_pdn_make_slashes_string test.cpp)
target_include_directories(test_pdn_make_slashes_string PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_make_slashes_string PRIVATE cxx_std_20)
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # pdn/spdn.h is a large translation unit; with debug information (-g) the
    # MinGW assembler needs big object support
    target_compile_options(test_pdn_make_slashes_string PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_make_slashes_string COMMAND test_pdn_make_slashes_string)