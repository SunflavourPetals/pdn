add_executable(test_pdn_proxy test.cpp)
target_include_directories(test_pdn_proxy PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_proxy PRIVATE cxx_std_20)
add_test(NAME pdn_proxy COMMAND test_pdn_proxy)
