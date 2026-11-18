add_executable(test_pdn_entity_utility test.cpp)
target_include_directories(test_pdn_entity_utility PRIVATE ${PDN_INCLUDE_DIR})
target_compile_features(test_pdn_entity_utility PRIVATE cxx_std_20)
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # this test includes pdn/spdn.h, whose template depth overflows the default COFF section count
    target_compile_options(test_pdn_entity_utility PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_entity_utility COMMAND test_pdn_entity_utility)
