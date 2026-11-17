#include <pdn/pdn_constant_generator.h>

#include <cmath>
#include <cstdio>
#include <numbers>
#include <optional>
#include <type_traits>
#include <utility>

namespace
{
    int fails = 0;

    void check(bool ok, const char* what)
    {
        if (!ok)
        {
            ++fails;
            std::printf("FAILED: %s\n", what);
        }
    }

    using pdn::concepts::constant_generator;
    using pdn::unicode::u16char_t;
    using pdn::unicode::u32char_t;
    using pdn::unicode::u8char_t;
    using pdn::unicode::u8string;
    using gen_u8 = pdn::default_constant_generator<u8char_t>;
    using gen_u16 = pdn::default_constant_generator<u16char_t>;
    using gen_u32 = pdn::default_constant_generator<u32char_t>;
    using entity_u8 = pdn::entity<u8char_t>;
    using entity_u16 = pdn::entity<u16char_t>;
    using entity_u32 = pdn::entity<u32char_t>;

    // The adapter satisfies the generator concept for every character width entity supports.
    static_assert(constant_generator<gen_u8, u8char_t>);
    static_assert(constant_generator<gen_u16, u16char_t>);
    static_assert(constant_generator<gen_u32, u32char_t>);

    // The result type is exactly std::optional<entity<char_t>>: no reference, no other character width.
    static_assert(std::is_same_v<decltype(gen_u8::generate_constant(std::declval<const u8string&>())), std::optional<entity_u8>>);
    static_assert(std::is_same_v<decltype(gen_u16::generate_constant(std::declval<const u8string&>())), std::optional<entity_u16>>);
    static_assert(std::is_same_v<decltype(gen_u32::generate_constant(std::declval<const u8string&>())), std::optional<entity_u32>>);

    // generate_constant is static, so it is callable without an object (the concept probes it through one).
    static_assert(std::is_invocable_r_v<std::optional<entity_u8>, decltype(gen_u8::generate_constant), const u8string&>);

    void test_standard_constants()
    {
        const auto pi = gen_u8::generate_constant(u8"pi");
        check(pi.has_value(), "pi is a known constant");
        check(pi && pi->as_fp() == std::numbers::pi, "pi value");

        const auto i8_min = gen_u8::generate_constant(u8"i8_min");
        check(i8_min.has_value(), "i8_min is a known constant");
        check(i8_min && i8_min->as_int() == -128, "i8_min value");

#ifndef PDN_NO_EXTENSIONS
        const auto hello = gen_u8::generate_constant(u8"hello");
        check(hello.has_value(), "hello is a known constant");
        check(hello && hello->as_string() == u8"Hello, world!", "hello value");

        const auto fib = gen_u8::generate_constant(u8"fib_10_list");
        check(fib.has_value(), "fib_10_list is a known constant");
        check(fib && fib->as_list().size() == 10, "fib_10_list size");
        check(fib && fib->as_list().back().as_int() == 55, "fib_10_list last element");

        const auto me = gen_u8::generate_constant(u8"me_object");
        check(me.has_value(), "me_object is a known constant");
        check(me && me->as_object().size() == 2, "me_object size");
#else
        // The six non-standard constants are absent when PDN_NO_EXTENSIONS is defined.
        check(!gen_u8::generate_constant(u8"hello").has_value(), "hello is an extension");
        check(!gen_u8::generate_constant(u8"fib_10_list").has_value(), "fib_10_list is an extension");
        check(!gen_u8::generate_constant(u8"me_object").has_value(), "me_object is an extension");
#endif

        const auto infinity = gen_u8::generate_constant(u8"infinity");
        check(infinity.has_value(), "infinity is a known constant");
        check(infinity && std::isinf(infinity->as_fp()), "infinity value");

        const auto qnan = gen_u8::generate_constant(u8"qNaN");
        check(qnan.has_value(), "qNaN is a known constant");
        check(qnan && std::isnan(qnan->as_fp()), "qNaN value");
    }

    void test_unknown_constants()
    {
        check(!gen_u8::generate_constant(u8"zzz").has_value(), "unknown identifier returns nullopt");
        check(!gen_u8::generate_constant(u8string{}).has_value(), "empty identifier returns nullopt");
        check(!gen_u8::generate_constant(u8"PI").has_value(), "lookup is case sensitive");
        check(!gen_u8::generate_constant(u8"pi ").has_value(), "no trailing space tolerance");
    }

    void test_other_char_types()
    {
        // The key is UTF-8 text for every target character type: a u16/u32 entity is produced from a
        // u8string key, not from a u16/u32 key.
        const auto pi16 = gen_u16::generate_constant(u8"pi");
        check(pi16.has_value(), "u16 generator finds pi from a UTF-8 key");
        check(pi16 && pi16->as_fp() == std::numbers::pi, "u16 pi value");

        const auto i8_min32 = gen_u32::generate_constant(u8"i8_min");
        check(i8_min32.has_value(), "u32 generator finds i8_min from a UTF-8 key");
        check(i8_min32 && i8_min32->as_int() == -128, "u32 i8_min value");

        check(!gen_u16::generate_constant(u8"zzz").has_value(), "u16 unknown identifier returns nullopt");
        check(!gen_u32::generate_constant(u8"zzz").has_value(), "u32 unknown identifier returns nullopt");
    }

    void test_call_forms()
    {
        gen_u8 generator{};
        check(generator.generate_constant(u8"pi").has_value(), "call through an instance");

#ifndef PDN_NO_EXTENSIONS
        u8string iden = u8"hello";
        const auto by_lvalue = gen_u8::generate_constant(iden);
        check(by_lvalue && by_lvalue->as_string() == u8"Hello, world!", "call with an lvalue key");
#else
        u8string iden = u8"pi";
        const auto by_lvalue = gen_u8::generate_constant(iden);
        check(by_lvalue && by_lvalue->as_fp() == std::numbers::pi, "call with an lvalue key");
#endif

        const auto first = gen_u8::generate_constant(u8"i8_min");
        const auto second = gen_u8::generate_constant(u8"i8_min");
        check(first && second && first->as_int() == second->as_int(), "each call returns its own copy");
    }
}

int main()
{
    test_standard_constants();
    test_unknown_constants();
    test_other_char_types();
    test_call_forms();

    if (fails != 0)
    {
        std::printf("%d check(s) failed\n", fails);
        return 1;
    }
    std::printf("all pdn_constant_generator checks passed\n");
    return 0;
}
