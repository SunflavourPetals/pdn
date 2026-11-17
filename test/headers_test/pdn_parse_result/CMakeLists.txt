add_executable(test_pdn_parse_result test.cpp)
target_include_directories(test_pdn_parse_result PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_parse_result PRIVATE cxx_std_20)
add_test(NAME pdn_parse_result COMMAND test_pdn_parse_result)

# Reading value()/operator*/operator-> from a failed result asserts in debug builds; the release
# fallback (the default entity) only exists when NDEBUG is defined, so it gets its own target.
add_executable(test_pdn_parse_result_ndebug test_ndebug.cpp)
target_include_directories(test_pdn_parse_result_ndebug PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_parse_result_ndebug PRIVATE cxx_std_20)
target_compile_definitions(test_pdn_parse_result_ndebug PRIVATE NDEBUG)
add_test(NAME pdn_parse_result_ndebug COMMAND test_pdn_parse_result_ndebug)
