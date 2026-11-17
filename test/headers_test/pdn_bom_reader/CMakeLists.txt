add_executable(test_pdn_bom_reader test.cpp)
target_include_directories(test_pdn_bom_reader PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_bom_reader PRIVATE cxx_std_20)
add_test(NAME pdn_bom_reader COMMAND test_pdn_bom_reader)
