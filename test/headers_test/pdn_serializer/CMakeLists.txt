add_executable(test_pdn_serializer test.cpp)
target_include_directories(test_pdn_serializer PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_serializer PRIVATE cxx_std_20)
# this TU pulls in pdn/spdn.h; the MinGW assembler rejects the resulting object
# as "file too big" at -O0, so ask GNU as for COFF big objects
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(test_pdn_serializer PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_serializer COMMAND test_pdn_serializer)
