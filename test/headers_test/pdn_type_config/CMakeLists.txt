add_executable(test_pdn_type_config test.cpp)
target_include_directories(test_pdn_type_config PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_type_config PRIVATE cxx_std_20)
add_test(NAME pdn_type_config COMMAND test_pdn_type_config)

# Same source, compiled with the ordered map configuration.
add_executable(test_pdn_type_config_ordered_map test.cpp)
target_include_directories(test_pdn_type_config_ordered_map PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_type_config_ordered_map PRIVATE cxx_std_20)
target_compile_definitions(test_pdn_type_config_ordered_map PRIVATE PDN_USER_OBJECT=::pdn::ordered_map)
add_test(NAME pdn_type_config_ordered_map COMMAND test_pdn_type_config_ordered_map)
