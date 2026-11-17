add_executable(test_pdn_unicode_xid test.cpp)
target_include_directories(test_pdn_unicode_xid PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_unicode_xid PRIVATE cxx_std_20)
# the test also parses through the public entry points, whose object file the
# MinGW assembler rejects as "file too big" without COFF big objects
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(test_pdn_unicode_xid PRIVATE "-Wa,-mbig-obj")
endif()
add_test(NAME pdn_unicode_xid COMMAND test_pdn_unicode_xid)
