add_executable(test_pdn_parser test.cpp)
target_include_directories(test_pdn_parser PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_parser PRIVATE cxx_std_20)
add_test(NAME pdn_parser COMMAND test_pdn_parser)
