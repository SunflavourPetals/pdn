#include <pdn/pdn_constant_generator_concept.h>

#include <pdn/pdn_entity.h>

#include <cstdio>
#include <optional>
#include <string_view>
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
    using pdn::unicode::u8string;
    using pdn::unicode::u16char_t;
    using pdn::unicode::u8char_t;
    using entity_u8 = pdn::entity<u8char_t>;
    using entity_u16 = pdn::entity<u16char_t>;

    // A generator that takes the identifier by value; it satisfies both the lvalue and the rvalue probe.
    struct by_value_gen
    {
        auto generate_constant(u8string iden) -> std::optional<entity_u8>
        {
            if (iden == u8"answer")
            {
                return entity_u8{ pdn::type::i32{ 42 } };
            }
            return std::nullopt;
        }
    };

    // A const reference parameter also satisfies both probes.
    struct const_ref_gen
    {
        auto generate_constant(const u8string& iden) const -> std::optional<entity_u8>
        {
            if (iden == u8"answer")
            {
                return entity_u8{ pdn::type::i32{ 42 } };
            }
            return std::nullopt;
        }
    };

    // The built-in generator is a static member function; calling it through an object is legal.
    struct static_gen
    {
        static auto generate_constant(const u8string& iden) -> std::optional<entity_u16>
        {
            if (iden == u8"answer")
            {
                return entity_u16{ pdn::type::u16{ 1 } };
            }
            return std::nullopt;
        }
    };

    // Returning a reference to the optional is accepted too: remove_cvref_same_as ignores cv and references.
    struct ref_result_gen
    {
        auto generate_constant(const u8string& iden) -> const std::optional<entity_u8>&
        {
            (void)iden;
            return cache;
        }
        static inline std::optional<entity_u8> cache{};
    };

    // A parameter that accepts a u8string by conversion works as well: the probe only requires the call to
    // be well formed.
    struct view_param_gen
    {
        auto generate_constant(std::u8string_view iden) -> std::optional<entity_u8>
        {
            (void)iden;
            return std::nullopt;
        }
    };

    // Rejected: only the rvalue overload exists, so the lvalue probe fails - and the parser passes a lvalue.
    struct rvalue_only_gen
    {
        auto generate_constant(u8string&& iden) -> std::optional<entity_u8>
        {
            (void)iden;
            return std::nullopt;
        }
    };

    // Rejected: other character type, plain entity instead of optional, no such member.
    struct wrong_char_gen
    {
        auto generate_constant(const u8string& iden) -> std::optional<entity_u16>
        {
            (void)iden;
            return std::nullopt;
        }
    };
    struct not_optional_gen
    {
        auto generate_constant(const u8string& iden) -> entity_u8
        {
            (void)iden;
            return entity_u8{};
        }
    };
    struct no_member_gen
    {
    };

    static_assert(constant_generator<by_value_gen, u8char_t>);
    static_assert(constant_generator<const_ref_gen, u8char_t>);
    static_assert(constant_generator<static_gen, u16char_t>);
    static_assert(constant_generator<ref_result_gen, u8char_t>);
    static_assert(constant_generator<view_param_gen, u8char_t>);

    static_assert(!constant_generator<rvalue_only_gen, u8char_t>);
    static_assert(!constant_generator<wrong_char_gen, u8char_t>);
    static_assert(!constant_generator<not_optional_gen, u8char_t>);
    static_assert(!constant_generator<no_member_gen, u8char_t>);
    static_assert(!constant_generator<int, u8char_t>);
    static_assert(!constant_generator<void, u8char_t>);

    // The concept is per character type: the same generator may satisfy another target type.
    static_assert(constant_generator<wrong_char_gen, u16char_t>);
    static_assert(!constant_generator<static_gen, u8char_t>);

    // Exercise the concept through a constrained helper, the way the parser does.
    template <typename gen_t>
    auto lookup(gen_t& gen, const u8string& iden) -> std::optional<entity_u8>
        requires constant_generator<gen_t, u8char_t>
    {
        return gen.generate_constant(iden);
    }

    void test_dispatch()
    {
        by_value_gen by_value;
        const_ref_gen by_ref;
        const std::optional<entity_u8> hit = lookup(by_value, u8"answer");
        const std::optional<entity_u8> miss = lookup(by_value, u8"absent");
        check(hit.has_value() && hit->as_int() == 42, "by_value_gen finds the constant");
        check(!miss.has_value(), "by_value_gen reports unknown with nullopt");
        check(lookup(by_ref, u8"answer")->as_int() == 42, "const_ref_gen finds the constant");
    }
}

int main()
{
    test_dispatch();
    if (fails != 0)
    {
        std::printf("%d check(s) failed\n", fails);
        return 1;
    }
    std::printf("probe ok\n");
    return 0;
}
