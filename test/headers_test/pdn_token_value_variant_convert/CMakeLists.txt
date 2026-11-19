add_executable(test_pdn_token_value_variant_convert test.cpp)
target_include_directories(test_pdn_token_value_variant_convert PRIVATE ${PDN_INCLUDE_DIR})
target_compile_features(test_pdn_token_value_variant_convert PRIVATE cxx_std_20)
if(MSVC)
    # This test contains real UTF-8 string literals; keep the source untouched.
    target_compile_options(test_pdn_token_value_variant_convert PRIVATE "/utf-8")
endif()
add_test(NAME pdn_token_value_variant_convert COMMAND test_pdn_token_value_variant_convert)
