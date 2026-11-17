add_executable(test_pdn_false test.cpp)
target_include_directories(test_pdn_false PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_false PRIVATE cxx_std_20)
add_test(NAME pdn_false COMMAND test_pdn_false)
