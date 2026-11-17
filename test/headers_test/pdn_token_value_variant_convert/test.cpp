#include <concepts>
#include <cstdio>
#include <string_view>
#include <variant>

#include <pdn/pdn_token_value_variant_convert.h>

namespace
{
    int fails = 0;

    #define CHECK(condition) \
        do { if (!(condition)) { ::std::printf("FAILED: %s  (line %d)\n", #condition, __LINE__); ++fails; } } while (false)

    using namespace pdn;
    using namespace pdn::literals::unicode_literals;
    using u8c  = unicode::u8char_t;
    using u16c = unicode::u16char_t;
    using u32c = unicode::u32char_t;
    using v8   = token_value_variant<u8c>;
    using v16  = token_value_variant<u16c>;
    using v32  = token_value_variant<u32c>;

    static_assert(::std::same_as<decltype(detail::token_value_variant_convert<u16c>(v8{})), v16>);
    static_assert(::std::same_as<decltype(detail::token_value_variant_convert<u8c>(v8{})), v8>);

    // the same-character-type call is a pass-through usable in constant expressions
    constexpr v8 same = detail::token_value_variant_convert<u8c>(v8{ type::i32{ 9 } });
    static_assert(::std::get<type::i32>(same) == 9);
    static_assert(same.index() == 3);   // monostate, i8, i16, i32, ...

    void test_scalars()
    {
        const v8 sv{ type::i8{ -3 } };
        const auto to16 = detail::token_value_variant_convert<u16c>(sv);
        CHECK(to16.index() == sv.index());
        CHECK(::std::get<type::i8>(to16) == type::i8{ -3 });

        const v8 fv{ type::f64{ 0.5 } };
        CHECK(::std::get<type::f64>(detail::token_value_variant_convert<u32c>(fv)) == 0.5);
        CHECK(::std::get<bool>(detail::token_value_variant_convert<u16c>(v8{ true })) == true);
        CHECK(detail::token_value_variant_convert<u16c>(v8{}).index() == 0);
    }

    void test_character()
    {
        const v8 ascii{ type::character<u8c>{ u8"a"_s.cbegin(), 1 } };
        CHECK(::std::get<type::character<u16c>>(detail::token_value_variant_convert<u16c>(ascii)).size() == 1);

        const v8 cjk{ type::character<u8c>{ u8"中"_s.cbegin(), 3 } };
        CHECK(::std::get<type::character<u32c>>(detail::token_value_variant_convert<u32c>(cjk)).size() == 1);

        const v8 emoji{ type::character<u8c>{ u8"😀"_s.cbegin(), 4 } };
        const auto e16 = detail::token_value_variant_convert<u16c>(emoji);
        CHECK(::std::get<type::character<u16c>>(e16).size() == 2);   // surrogate pair

        const v8 empty{ type::character<u8c>{ u8""_s.cbegin(), 0 } };
        CHECK(::std::get<type::character<u16c>>(detail::token_value_variant_convert<u16c>(empty)).size() == 0);

        const char8_t lone[] = { 0xED, 0xA0, 0x80 };              // lone high surrogate
        const v8 broken{ type::character<u8c>{ lone, 3 } };
        CHECK(::std::get<type::character<u32c>>(detail::token_value_variant_convert<u32c>(broken)).size() == 1);
    }

    void test_string()
    {
        const v8 cjk{ make_proxy<type::u8string>(u8"中文"_sv) };
        const auto c16 = detail::token_value_variant_convert<u16c>(cjk);
        CHECK(::std::get<proxy<type::u16string>>(c16)->size() == 2);
        CHECK(c16.index() == cjk.index());

        const v8 empty{ make_proxy<type::u8string>(u8""_sv) };
        CHECK(::std::get<proxy<type::u32string>>(detail::token_value_variant_convert<u32c>(empty))->empty());
    }

    void test_at_identifier()
    {
        const v8 at{ detail::at_iden_string_proxy{ u8"i8_max"_sv } };
        const auto at16 = detail::token_value_variant_convert<u16c>(at);
        CHECK(at16.index() == 14);
        CHECK(::std::get<detail::at_iden_string_proxy>(at16).get_id() == u8"i8_max"_sv);
        const auto at32 = detail::token_value_variant_convert<u32c>(at);
        CHECK(::std::get<detail::at_iden_string_proxy>(at32).get_id() == u8"i8_max"_sv);
    }
}

int main()
{
    test_scalars();
    test_character();
    test_string();
    test_at_identifier();
    if (fails != 0)
    {
        ::std::printf("pdn_token_value_variant_convert: %d failure(s)\n", fails);
        return 1;
    }
    ::std::printf("pdn_token_value_variant_convert ok\n");
    return 0;
}
