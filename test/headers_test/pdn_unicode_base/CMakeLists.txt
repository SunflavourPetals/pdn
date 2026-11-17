add_executable(test_pdn_unicode_base test.cpp)
target_include_directories(test_pdn_unicode_base PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_unicode_base PRIVATE cxx_std_20)
add_test(NAME pdn_unicode_base COMMAND test_pdn_unicode_base)
