#include <cstddef>
#include <cstdio>
#include <string>
#include <type_traits>

#include <pdn/pdn_token.h>
#include <pdn/pdn_token_convert.h>
#include <pdn/pdn_type.h>

namespace
{
    int fails = 0;

    #define CHECK(condition) \
        do { if (!(condition)) { ::std::printf("FAILED: %s  (line %d)\n", #condition, __LINE__); ++fails; } } while (false)

    using namespace pdn::literals::unicode_literals;
    using pdn::pdn_token_code;
    using u8c  = pdn::unicode::u8char_t;
    using u16c = pdn::unicode::u16char_t;
    using u32c = pdn::unicode::u32char_t;

    using t8  = pdn::token<u8c>;
    using t16 = pdn::token<u16c>;
    using t32 = pdn::token<u32c>;

    static_assert(::std::is_aggregate_v<t8>);
    static_assert(::std::is_standard_layout_v<t8>);
    static_assert(!::std::is_trivially_copyable_v<t8>);   // the payload variant owns proxies
    static_assert(::std::is_copy_constructible_v<t8>);
    static_assert(::std::is_nothrow_move_constructible_v<t8>);
    static_assert(::std::is_nothrow_move_assignable_v<t8>);
    static_assert(sizeof(t8) == 40 && alignof(t8) == 8);
    static_assert(sizeof(t16) == 40 && sizeof(t32) == 40);
    static_assert(offsetof(t8, position) == 0);
    static_assert(offsetof(t8, code) == 16);
    static_assert(offsetof(t8, value) == 24);                // 4 bytes of padding after code

    void test_default_token()
    {
        const t8 tk{};
        CHECK(tk.position.line == 1 && tk.position.column == 1);
        CHECK(tk.code == pdn_token_code::invalid);
        CHECK(tk.value.index() == 0);                       // monostate
    }

    void test_initialization()
    {
        const t8 designated{ .position = { 3, 7 }, .code = pdn_token_code::literal_integer,
                             .value = pdn::type::i32{ 42 } };
        CHECK(designated.position.line == 3 && designated.position.column == 7);
        CHECK(designated.code == pdn_token_code::literal_integer);
        CHECK(::std::get<pdn::type::i32>(designated.value) == 42);

        const t8 positional{ { 4, 2 }, pdn_token_code::eof, {} };
        CHECK(positional.position.line == 4 && positional.position.column == 2);
        CHECK(positional.code == pdn_token_code::eof);
        CHECK(positional.value.index() == 0);

        const t8 copied = designated;
        CHECK(copied.position == designated.position);
        CHECK(copied.code == designated.code);
        CHECK(::std::get<pdn::type::i32>(copied.value) == 42);
    }

    void test_payloads()
    {
        t8 with_string{ .position = { 1, 5 }, .code = pdn_token_code::literal_string,
                        .value = pdn::make_proxy<pdn::type::u8string>(u8"abc"_sv) };
        CHECK(::std::get<pdn::proxy<pdn::type::u8string>>(with_string.value)->size() == 3);

        t8 with_at{ .position = { 2, 1 }, .code = pdn_token_code::at_identifier,
                    .value = pdn::detail::at_iden_string_proxy{ u8"i8_max"_sv } };
        CHECK(::std::get<pdn::detail::at_iden_string_proxy>(with_at.value).get_id() == u8"i8_max"_sv);

        // the parser synthesizes the eof token like this, keeping the last position:
        t8 last = with_string;
        last = pdn::token<u8c>{ .position = last.position, .code = pdn_token_code::eof, .value = {} };
        CHECK(last.code == pdn_token_code::eof);
        CHECK(last.value.index() == 0);
        CHECK(last.position.line == 1 && last.position.column == 5);
    }

    void test_token_convert()
    {
        const t8 src{ .position = { 9, 3 }, .code = pdn_token_code::literal_string,
                      .value = pdn::make_proxy<pdn::type::u8string>(u8"hello"_sv) };
        const t16 dst = pdn::detail::token_convert<u16c>(src);
        CHECK(dst.position == src.position);
        CHECK(dst.code == src.code);
        CHECK(::std::get<pdn::proxy<pdn::type::u16string>>(dst.value)->size() == 5);

        const t8 num{ { 1, 1 }, pdn_token_code::literal_integer, pdn::type::i64{ 7 } };
        const t32 num32 = pdn::detail::token_convert<u32c>(num);
        CHECK(num32.code == pdn_token_code::literal_integer);
        CHECK(::std::get<pdn::type::i64>(num32.value) == 7);

        const t8 same = pdn::detail::token_convert<u8c>(num);
        CHECK(same.position == num.position && same.code == num.code);
    }
}

int main()
{
    test_default_token();
    test_initialization();
    test_payloads();
    test_token_convert();
    if (fails != 0)
    {
        ::std::printf("pdn_token: %d failure(s)\n", fails);
        return 1;
    }
    ::std::printf("pdn_token ok\n");
    return 0;
}