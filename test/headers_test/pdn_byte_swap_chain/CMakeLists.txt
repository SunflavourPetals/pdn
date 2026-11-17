add_executable(test_pdn_byte_swap_chain test.cpp)
target_include_directories(test_pdn_byte_swap_chain PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_byte_swap_chain PRIVATE cxx_std_20)
add_test(NAME pdn_byte_swap_chain COMMAND test_pdn_byte_swap_chain)

# An empty buffer is a precondition violation asserted in debug builds; the release fallback (report
# eof() right away instead of spinning) only exists when NDEBUG is defined, so it gets its own target.
add_executable(test_pdn_byte_swap_chain_release test_release.cpp)
target_include_directories(test_pdn_byte_swap_chain_release PRIVATE "${PDN_INCLUDE_DIR}")
target_compile_features(test_pdn_byte_swap_chain_release PRIVATE cxx_std_20)
target_compile_definitions(test_pdn_byte_swap_chain_release PRIVATE NDEBUG)
add_test(NAME pdn_byte_swap_chain_release COMMAND test_pdn_byte_swap_chain_release)
