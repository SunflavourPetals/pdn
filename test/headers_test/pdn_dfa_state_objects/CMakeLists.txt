add_executable(test_pdn_dfa_state_objects test.cpp)
target_include_directories(test_pdn_dfa_state_objects PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_dfa_state_objects PRIVATE cxx_std_20)
add_test(NAME pdn_dfa_state_objects COMMAND test_pdn_dfa_state_objects)
