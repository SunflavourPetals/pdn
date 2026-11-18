add_executable(test_pdn_type_generator test.cpp)
target_include_directories(test_pdn_type_generator PRIVATE ${PDN_INCLUDE_DIR})
target_compile_features(test_pdn_type_generator PRIVATE cxx_std_20)
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # the translation unit pulls in pdn/spdn.h; MinGW needs bigger object files
    target_compile_options(test_pdn_type_generator PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_type_generator COMMAND test_pdn_type_generator)