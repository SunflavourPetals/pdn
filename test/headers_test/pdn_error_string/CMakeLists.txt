add_executable(test_pdn_error_string test.cpp)
target_include_directories(test_pdn_error_string PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_error_string PRIVATE cxx_std_20)
add_test(NAME pdn_error_string COMMAND test_pdn_error_string)
