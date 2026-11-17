#include <pdn/pdn_err_msg_gen_utility.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

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

    using pdn::error_msg_string;
    namespace util = pdn::detail::err_msg_gen_util;
    namespace syn = pdn::detail::err_msg_gen_util::syntax_err_msg_gen_util;
    namespace lex = pdn::detail::err_msg_gen_util::lexical_err_msg_gen_util;
    namespace rt = pdn::raw_error_message_type;
    using char_t = pdn::error_msg_char;
    using variant_t = pdn::raw_error_message_variant;
    using value_t = pdn::token_value_variant<char_t>;
    using character_t = pdn::type::character<char_t>;

    // offset_of_leading only needs these two members, so a small stand-in keeps the arithmetic
    // testable without running a decoder first.
    struct fake_result
    {
        ::std::size_t d;
        auto distance() const noexcept { return d; }
    };
    struct fake_msg
    {
        ::std::size_t last_code_unit_offset;
        fake_result result;
    };

    static_assert(::std::is_same_v<decltype(util::to_s<10>(0)), error_msg_string>);
    static_assert(::std::is_same_v<decltype(util::offset_of_leading(fake_msg{})), error_msg_string>);
    static_assert(::std::is_same_v<decltype(syn::get_integer_min_value(pdn::type_code::i8)), pdn::type::i64>);
    static_assert(::std::is_same_v<decltype(syn::get_integer_max_value(pdn::type_code::u8)), pdn::type::u64>);
    static_assert(::std::is_same_v<decltype(syn::get_integer_min_value_s(pdn::type_code::i8)), error_msg_string>);

    void test_quoting()
    {
        check(util::add_quote(u8"abc") == u8"\"abc\"", "add_quote");
        check(util::add_quote(u8"") == u8"\"\"", "add_quote empty");
        check(util::add_single_quote(u8"abc") == u8"'abc'", "add_single_quote");
    }

    void test_make_slashes()
    {
        check(util::make_slashes(u8"a\"b") == u8"a\\\"b", "make_slashes quote");
        check(util::make_slashes(u8"a\\b") == u8"a\\\\b", "make_slashes backslash");
        check(util::make_slashes(u8"a\tb") == u8"a\\tb", "make_slashes tab");
        check(util::make_slashes(u8"a\nb") == u8"a\\nb", "make_slashes line feed");
        check(util::make_slashes(u8"a'b") == u8"a\\'b", "make_slashes single quote");
        check(util::make_slashes(u8"a`b") == u8"a`b", "make_slashes keeps backtick");
        check(util::make_slashes(u8"\xe4\xb8\xad") == u8"\xe4\xb8\xad", "make_slashes keeps non ascii");
        // A non-scalar code point has no text form and is dropped, like the string escaper does.
        check(util::make_slashes(u8"a\x01" u8"b") == u8"a\\u0001b", "make_slashes escapes other control chars");
        check(util::make_slashes(u8"\x7f") == u8"\\u007f", "make_slashes escapes DEL");
        check(util::make_slashes(u8"\xc2\x85") == u8"\\u0085", "make_slashes escapes NEL");
        check(util::make_slashes(u8"\xef\xbb\xbf") == u8"\xef\xbb\xbf", "make_slashes keeps BOM sign");
    }

    void test_to_s()
    {
        check(util::to_s(0) == u8"0", "to_s zero");
        check(util::to_s(42) == u8"42", "to_s decimal");
        check(util::to_s(-42) == u8"-42", "to_s negative");
        check(util::to_s<16>(255) == u8"ff", "to_s hex");
        check(util::to_s<16, 4>(255) == u8"00ff", "to_s hex padded");
        check(util::to_s<16, 4>(42) == u8"002a", "to_s hex padded small");
        check(util::to_s<2>(5) == u8"101", "to_s binary");
        check(util::to_s<8>(8) == u8"10", "to_s octal");
        check(util::to_s<36>(35) == u8"z", "to_s base 36 last digit");
        check(util::to_s<36>(36) == u8"10", "to_s base 36 carry");
        check(util::to_s<10, 5>(42) == u8"00042", "to_s width pads zeros");
        // The padding goes after the sign, not in front of it.
        check(util::to_s<10, 5>(-42) == u8"-0042", "to_s width pads after sign");
        // filling replaces the default zero.
        check(util::to_s<10, 5, u8'_'>(-42) == u8"-__42", "to_s width fills with underscore");
        check(util::to_s<10, 5, u8' '>(-42) == u8"-  42", "to_s width fills with space");
        check(util::to_s<8, 6, u8'.'>(42) == u8"....52", "to_s width fills octal with dot");
        check(util::to_s<10, 1, u8'_'>(-42) == u8"-42", "to_s filling unused without width");
        check(util::to_s<10, 5>(123456) == u8"123456", "to_s width does not truncate");
        check(util::to_s((::std::numeric_limits<::std::int64_t>::min)()) == u8"-9223372036854775808", "to_s int64 min");
        check(util::to_s((::std::numeric_limits<::std::int64_t>::max)()) == u8"9223372036854775807", "to_s int64 max");
        check(util::to_s((::std::numeric_limits<::std::uint64_t>::max)()) == u8"18446744073709551615", "to_s uint64 max");
        check(util::to_s<2>(::std::int64_t{ -1 }) == u8"-1", "to_s binary negative");
        check(util::to_s(::std::uint8_t{ 200 }) == u8"200", "to_s unsigned char");
        check(util::to_s(::std::int8_t{ -128 }) == u8"-128", "to_s signed char");
    }

    void test_offset_of_leading()
    {
        // 0x10 - 2 = 14 = 0x0e; utf-8 counts bytes, utf-16 halves and utf-32 quarters.
        check(util::offset_of_leading(fake_msg{ 0x10, { 2 } }) == u8"0x000e", "offset utf8");
        check(util::offset_of_leading(fake_msg{ 0x10, { 2 } }, 2) == u8"0x001c", "offset utf16");
        check(util::offset_of_leading(fake_msg{ 0x10, { 2 } }, 4) == u8"0x0038", "offset utf32");
        check(util::offset_of_leading(fake_msg{ 0x0, { 0 } }) == u8"0x0000", "offset at start");
    }

    void test_token_value()
    {
        check(util::token_value_variant_to_s(value_t{ character_t{ u8"x", 1 } }) == u8"'x'", "value character");
        check(util::token_value_variant_to_s(value_t{ character_t{ u8"'", 1 } }) == u8"'\\''", "value character quote");
        check(util::token_value_variant_to_s(value_t{ character_t{ u8"\t", 1 } }) == u8"'\\t'", "value character tab");
        check(util::token_value_variant_to_s(value_t{ character_t{ u8"\x01", 1 } }) == u8"'\\u0001'", "value character control");
        check(util::token_value_variant_to_s(value_t{ pdn::make_proxy<pdn::type::string<char_t>>(u8"a\"b") })
            == u8"\"a\\\"b\"", "value string");
        check(util::token_value_variant_to_s(value_t{ pdn::make_proxy<pdn::type::string<char_t>>(u8"a\x01" u8"b") })
            == u8"\"a\\u0001b\"", "value string control");
        check(util::token_value_variant_to_s(value_t{ pdn::type::boolean{ true } }) == u8"true", "value true");
        check(util::token_value_variant_to_s(value_t{ pdn::type::boolean{ false } }) == u8"false", "value false");
        check(util::token_value_variant_to_s(value_t{ ::std::monostate{} }) == u8"monostate", "value monostate");
        check(util::token_value_variant_to_s(value_t{ pdn::detail::at_iden_string_proxy{ u8"i32_max" } })
            == u8"\"@i32_max\"", "value at identifier");
        check(util::token_value_variant_to_s(value_t{ pdn::type::i8{ -128 } }) == u8"-128", "value i8");
        check(util::token_value_variant_to_s(value_t{ pdn::type::i32{ 42 } }) == u8"42", "value i32");
        check(util::token_value_variant_to_s(value_t{ pdn::type::u64{ 18446744073709551615ull } })
            == u8"18446744073709551615", "value u64");
        check(util::token_value_variant_to_s(value_t{ pdn::type::i64{ (::std::numeric_limits<long long>::min)() } })
            == u8"-9223372036854775808", "value i64 min");
        check(util::token_value_variant_to_s(value_t{ pdn::type::u8{ 255 } }) == u8"255", "value u8");
        check(util::token_value_variant_to_s(value_t{ pdn::type::u16{ 65535 } }) == u8"65535", "value u16");
        check(util::token_value_variant_to_s(value_t{ pdn::type::u32{ 4294967295u } }) == u8"4294967295", "value u32");
        // Integers print as decimals and floating-point values in the shortest round-trip form;
        // both go through format, so PDN_USER_FORMAT covers the whole numeric branch.
        check(util::token_value_variant_to_s(value_t{ pdn::type::f32{ 3.5f } }) == u8"3.5", "value f32");
        check(util::token_value_variant_to_s(value_t{ pdn::type::f64{ 3.5 } }) == u8"3.5", "value f64");
        check(util::token_value_variant_to_s(value_t{ pdn::type::f32{ 0.1f } }) == u8"0.1", "value f32 0.1");
        check(util::token_value_variant_to_s(value_t{ pdn::type::f64{ 1e300 } }) == u8"1e+300", "value f64 1e300");
        check(util::token_value_variant_to_s(value_t{ pdn::type::f64{ ::std::numeric_limits<double>::infinity() } })
            == u8"inf", "value f64 inf");
        check(util::token_value_variant_to_s(value_t{ pdn::type::f64{ ::std::numeric_limits<double>::quiet_NaN() } })
            == u8"nan", "value f64 nan");

        check(util::is_mono(value_t{ ::std::monostate{} }), "is_mono monostate");
        check(!util::is_mono(value_t{ pdn::type::i32{ 0 } }), "is_mono value");
    }

    void test_description_of()
    {
        pdn::token<char_t> tk{ {}, pdn::pdn_token_code::literal_integer, pdn::type::i32{ 7 } };
        check(util::description_of(tk) == u8"integer-val with value 7", "description with value");
        tk.value = ::std::monostate{};
        check(util::description_of(tk) == u8"integer-val", "description without value");
    }

    void test_integer_range()
    {
        check(syn::get_integer_min_value(pdn::type_code::i8) == -128, "min i8");
        check(syn::get_integer_min_value(pdn::type_code::i16) == -32768, "min i16");
        check(syn::get_integer_min_value(pdn::type_code::i32) == -2147483648ll, "min i32");
        check(syn::get_integer_min_value(pdn::type_code::i64) == (::std::numeric_limits<::std::int64_t>::min)(), "min i64");
        check(syn::get_integer_min_value(pdn::type_code::u8) == 0, "min u8");
        check(syn::get_integer_min_value(pdn::type_code::u16) == 0, "min u16");
        check(syn::get_integer_min_value(pdn::type_code::u32) == 0, "min u32");
        check(syn::get_integer_min_value(pdn::type_code::u64) == 0, "min u64");

        check(syn::get_integer_max_value(pdn::type_code::i8) == 127, "max i8");
        check(syn::get_integer_max_value(pdn::type_code::i16) == 32767, "max i16");
        check(syn::get_integer_max_value(pdn::type_code::i32) == 2147483647, "max i32");
        check(syn::get_integer_max_value(pdn::type_code::i64) == (::std::numeric_limits<::std::int64_t>::max)(), "max i64");
        check(syn::get_integer_max_value(pdn::type_code::u8) == 255, "max u8");
        check(syn::get_integer_max_value(pdn::type_code::u16) == 65535, "max u16");
        check(syn::get_integer_max_value(pdn::type_code::u32) == 4294967295ull, "max u32");
        check(syn::get_integer_max_value(pdn::type_code::u64) == (::std::numeric_limits<::std::uint64_t>::max)(), "max u64");

        check(syn::get_integer_min_value_s(pdn::type_code::i8) == u8"-128", "min i8 text");
        check(syn::get_integer_max_value_s(pdn::type_code::u8) == u8"255", "max u8 text");
    }

    void test_casting()
    {
        variant_t raw{ rt::casting_msg{ pdn::type::i32{ 300 }, pdn::type_code::i32, pdn::type_code::u8 } };
        check(syn::get_casting_operand(raw) == u8"300", "cast operand");
        check(syn::get_source_type_name(raw) == u8"i32", "cast source");
        check(syn::get_target_type_name(raw) == u8"u8", "cast target");
        check(syn::get_target_min_s(raw) == u8"0", "cast target min");
        check(syn::get_target_max_s(raw) == u8"255", "cast target max");
        check(!syn::is_source_type_list_or_object(raw), "cast source is scalar");

        raw = rt::casting_msg{ ::std::monostate{}, pdn::type_code::list, pdn::type_code::i32 };
        check(syn::get_casting_operand(raw) == u8"list", "cast operand list");
        check(syn::is_source_type_list_or_object(raw), "cast source is list");

        raw = rt::casting_msg{ ::std::monostate{}, pdn::type_code::object, pdn::type_code::i32 };
        check(syn::get_casting_operand(raw) == u8"object", "cast operand object");
        check(syn::is_source_type_list_or_object(raw), "cast source is object");
    }

    void test_syntax_readers()
    {
        variant_t raw{ rt::identifier{ u8"a b" } };
        check(syn::get_slashes_iden(raw) == u8"a b", "identifier");
        check(syn::get_at_iden_s(raw) == u8"\"@a b\"", "at identifier");

        // a Bidi_Control code point in the name must not reorder the rest of the message
        raw = rt::identifier{ u8"a" u8"\u202e" u8"b" };
        check(syn::get_at_iden_s(raw) == u8"\"@a\\u202eb\"", "at identifier with bidi control");

        raw = rt::unary_operation{ pdn::type::i32{ 5 }, pdn::type_code::i32, true };
        check(syn::get_unary_operator_s(raw) == u8"operator-", "unary minus");
        check(syn::get_operand_type_name(raw) == u8"i32", "unary operand type");
        check(syn::get_description_for_unary_operation(raw) == u8"i32 with value 5", "unary description");

        raw = rt::unary_operation{ ::std::monostate{}, pdn::type_code::list, false };
        check(syn::get_unary_operator_s(raw) == u8"operator+", "unary plus");
        check(syn::is_operand_type_list_or_object(raw), "unary operand is list");
        check(syn::get_description_for_unary_operation(raw) == u8"list", "unary description without value");

        raw = rt::error_token{ pdn::token<char_t>{ {}, pdn::pdn_token_code::literal_string, pdn::type::i32{ 3 } } };
        check(syn::get_description_for_error_token(raw) == u8"string-val with value 3", "error token");
    }

    void test_lexical_readers()
    {
        variant_t raw{ rt::not_unicode_scalar_value{ 0xD800u } };
        check(lex::get_code_point_hex(raw) == u8"0x0000d800", "code point hex");

        raw = rt::error_string{ u8"a\tb" };
        check(lex::get_err_s(raw) == u8"a\tb", "error_string text");
        check(lex::get_quoted_slashes_s(raw) == u8"\"a\\tb\"", "error_string quoted");
        check(lex::get_esc_quoted_s(raw) == u8"\"\\a\\tb\"", "error_string quoted with backslash");
        check(lex::get_single_quoted_slashes_s(raw) == u8"'a\\tb'", "error_string single quoted");

        raw = rt::character_length_error{ u8"abc", 3 };
        check(lex::get_cle_cp_length(raw) == u8"3", "character length");
        check(lex::get_single_quoted_slashes_s_for_cle(raw) == u8"'abc'", "character length text");

        raw = rt::missing_terminating_sequence{ u8"body", u8"\"##", true };
        check(lex::get_quoted_slashes_s_for_mts(raw) == u8"\"@`\\\"##(body\"", "missing terminating sequence");
        check(lex::get_slashes_d_seq_for_mts(raw) == u8"\\\"##", "missing terminating sequence delimiter");

        raw = rt::number_end_with_separator{ u8"1.5", rt::number_type::dec_floating };
        check(lex::get_s_for_ews(raw) == u8"1.5", "number end with separator");

        raw = rt::from_chars_error{ u8"12x", 2, ::std::errc::invalid_argument, rt::number_type::dec_integer };
        check(lex::get_quoted_from_chars_src_s(raw) == u8"\"12x\"", "from_chars source");
        check(lex::get_quoted_from_chars_incomplete_s(raw) == u8"\"12\"", "from_chars incomplete");
        check(lex::get_from_chars_desc_s(raw) == u8"integer, base 10", "from_chars description");
        check(lex::get_from_chars_errc_s(raw) == u8"22", "from_chars errc");

        struct desc_case { rt::number_type type; const char8_t* text; };
        const desc_case desc_cases[] = {
            { rt::number_type::bin_integer,  u8"integer, base 2" },
            { rt::number_type::oct_integer,  u8"integer, base 8" },
            { rt::number_type::dec_integer,  u8"integer, base 10" },
            { rt::number_type::hex_integer,  u8"integer, base 16" },
            { rt::number_type::dec_floating, u8"floating, default chars_format(general)" },
            { rt::number_type::hex_floating, u8"floating, chars_format hex" },
        };
        for (const auto& c : desc_cases)
        {
            raw = rt::from_chars_error{ u8"1", 1, ::std::errc::result_out_of_range, c.type };
            check(lex::get_from_chars_desc_s(raw) == c.text, "from_chars description of every number_type");
        }

        raw = rt::escape_not_unicode_scalar_value{ u8"u{110000}", 0x110000u };
        check(lex::get_value_from_esc_not_u_scalar(raw) == u8"0x110000", "escape value");
        check(lex::get_quoted_seq_from_esc_not_u_scalar(raw) == u8"\"\\u{110000}\"", "escape sequence");

        raw = rt::delimiter_error{ u8"\"##", false };
        check(lex::get_prefix_and_d_seq(raw) == u8"@\\\"\\\"##", "delimiter");
        check(lex::get_prefix_and_d_seq_with_par(raw) == u8"@\\\"\\\"##(", "delimiter with parenthesis");

        raw = rt::delimiter_error{ u8"`##", true };
        // make_slashes escapes quotes and backslashes but not the backtick, so both the prefix and
        // the delimiter are kept as they are.
        check(lex::get_prefix_and_d_seq(raw) == u8"@``##", "raw identifier delimiter");
    }
}

int main()
{
    test_quoting();
    test_make_slashes();
    test_to_s();
    test_offset_of_leading();
    test_token_value();
    test_description_of();
    test_integer_range();
    test_casting();
    test_syntax_readers();
    test_lexical_readers();
    if (fails == 0)
    {
        std::printf("all pdn_err_msg_gen_utility checks passed\n");
    }
    return fails == 0 ? 0 : 1;
}
