add_executable(test_pdn_ordered_map test.cpp)
target_include_directories(test_pdn_ordered_map PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_ordered_map PRIVATE cxx_std_20)
add_test(NAME pdn_ordered_map COMMAND test_pdn_ordered_map)
