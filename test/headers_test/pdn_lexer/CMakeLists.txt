add_executable(test_pdn_lexer test.cpp)
target_include_directories(test_pdn_lexer PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_lexer PRIVATE cxx_std_20)
add_test(NAME pdn_lexer COMMAND test_pdn_lexer)
