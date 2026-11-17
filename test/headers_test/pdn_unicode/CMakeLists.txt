add_executable(test_pdn_unicode test.cpp)
target_include_directories(test_pdn_unicode PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_unicode PRIVATE cxx_std_20)
add_test(NAME pdn_unicode COMMAND test_pdn_unicode)
