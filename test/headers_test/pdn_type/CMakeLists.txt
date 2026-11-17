add_executable(test_pdn_type test.cpp)
target_include_directories(test_pdn_type PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_type PRIVATE cxx_std_20)
add_test(NAME pdn_type COMMAND test_pdn_type)

add_executable(test_pdn_type_use test_use.cpp)
target_include_directories(test_pdn_type_use PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_type_use PRIVATE cxx_std_20)
add_test(NAME pdn_type_use COMMAND test_pdn_type_use)