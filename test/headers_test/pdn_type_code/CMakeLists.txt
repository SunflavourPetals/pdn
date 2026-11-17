add_executable(test_pdn_type_code test.cpp)
target_include_directories(test_pdn_type_code PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_type_code PRIVATE cxx_std_20)
add_test(NAME pdn_type_code COMMAND test_pdn_type_code)
