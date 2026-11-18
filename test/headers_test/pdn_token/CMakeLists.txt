add_executable(test_pdn_token test.cpp)
target_include_directories(test_pdn_token PRIVATE ${PDN_INCLUDE_DIR})
target_compile_features(test_pdn_token PRIVATE cxx_std_20)
add_test(NAME pdn_token COMMAND test_pdn_token)