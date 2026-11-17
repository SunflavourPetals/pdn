#include <pdn/pdn_raw_error_message_type.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <type_traits>

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

    namespace ret = pdn::raw_error_message_type;
    using namespace pdn::unicode_literals; // u8"_s
    using pdn::error_msg_string;

    // Every payload is a plain aggregate: the reporting site fills the fields it knows and the message
    // generator reads them back.
    static_assert(std::is_aggregate_v<ret::utf8_decode_error>);
    static_assert(std::is_aggregate_v<ret::utf16_decode_error>);
    static_assert(std::is_aggregate_v<ret::utf32_decode_error>);
    static_assert(std::is_aggregate_v<ret::identifier>);
    static_assert(std::is_aggregate_v<ret::error_token>);
    static_assert(std::is_aggregate_v<ret::unary_operation>);
    static_assert(std::is_aggregate_v<ret::casting_msg>);
    static_assert(std::is_aggregate_v<ret::not_unicode_scalar_value>);
    static_assert(std::is_aggregate_v<ret::error_string>);
    static_assert(std::is_aggregate_v<ret::character_length_error>);
    static_assert(std::is_aggregate_v<ret::number_end_with_separator>);
    static_assert(std::is_aggregate_v<ret::from_chars_error>);
    static_assert(std::is_aggregate_v<ret::escape_not_unicode_scalar_value>);
    static_assert(std::is_aggregate_v<ret::missing_terminating_sequence>);
    static_assert(std::is_aggregate_v<ret::delimiter_error>);

    // The decoding payloads only hold value types, so they stay trivially copyable.
    static_assert(std::is_trivially_copyable_v<ret::utf8_decode_error>);
    static_assert(!std::is_trivially_copyable_v<ret::error_string>);
    static_assert(!std::is_trivially_copyable_v<ret::error_token>);

    // Field types are the shared vocabulary of the error reporting layer.
    static_assert(std::is_same_v<decltype(ret::utf8_decode_error::result), pdn::unicode::utf8::decode_result>);
    static_assert(std::is_same_v<decltype(ret::utf16_decode_error::last_code_unit), pdn::unicode::utf16::code_unit_t>);
    static_assert(std::is_same_v<decltype(ret::from_chars_error::offset), std::ptrdiff_t>);
    static_assert(std::is_same_v<decltype(ret::from_chars_error::ec), std::errc>);
    static_assert(std::is_same_v<decltype(ret::character_length_error::code_point_count), std::size_t>);
    static_assert(std::is_same_v<decltype(ret::casting_msg::source_type), pdn::type_code>);

    // number_type names the literal syntax the number came from; the message only needs the spelling.
    static_assert(static_cast<int>(ret::number_type::bin_integer) == 0);
    static_assert(static_cast<int>(ret::number_type::oct_integer) == 1);
    static_assert(static_cast<int>(ret::number_type::dec_integer) == 2);
    static_assert(static_cast<int>(ret::number_type::hex_integer) == 3);
    static_assert(static_cast<int>(ret::number_type::dec_floating) == 4);
    static_assert(static_cast<int>(ret::number_type::hex_floating) == 5);

    void test_payload_values()
    {
        const ret::identifier id{ .value = u8"name"_s };
        check(id.value == u8"name", "identifier keeps the text");

        const ret::character_length_error cle{ .value = u8"'ab'"_s, .code_point_count = 2 };
        check(cle.value == u8"'ab'" && cle.code_point_count == 2, "character_length_error fields");

        const ret::number_end_with_separator nes{ .number_sequence = u8"12'"_s, .literal_type = ret::number_type::dec_integer };
        check(nes.number_sequence == u8"12'" && nes.literal_type == ret::number_type::dec_integer, "number_end_with_separator fields");

        const ret::from_chars_error fce{ .sequence = u8"12x"_s, .offset = 2, .ec = std::errc::invalid_argument, .type = ret::number_type::hex_integer };
        check(fce.offset == 2 && fce.ec == std::errc::invalid_argument && fce.type == ret::number_type::hex_integer, "from_chars_error fields");

        const ret::escape_not_unicode_scalar_value esc{ .escape_sequence = u8"u{D800}"_s, .code_point = 0xD800 };
        check(esc.escape_sequence == u8"u{D800}" && esc.code_point == 0xD800, "escape_not_unicode_scalar_value fields");

        const ret::missing_terminating_sequence mts{ .content = u8"abc"_s, .d_seq = u8"#$"_s, .is_raw_identifier_string = true };
        check(mts.d_seq == u8"#$" && mts.is_raw_identifier_string, "missing_terminating_sequence fields");

        const ret::delimiter_error de{ .d_seq = u8"|", .is_raw_identifier_string = false };
        check(de.d_seq == u8"|" && !de.is_raw_identifier_string, "delimiter_error fields");

        const ret::not_unicode_scalar_value ns{ .value = 0x110000 };
        check(ns.value == 0x110000, "not_unicode_scalar_value field");

        const ret::error_string es{ .value = u8"text"_s };
        check(es.value == u8"text", "error_string field");
    }

    void test_codec_payload()
    {
        constexpr auto bytes = std::array<pdn::unicode::u8char_t, 1>{ 0xFF };
        const auto decoded = pdn::unicode::utf8::decode<true>(bytes.data(), bytes.data() + 1);
        const ret::utf8_decode_error err{ .result = decoded, .last_code_unit = 0xFF, .last_code_unit_offset = 0 };
        check(err.result.errc() == pdn::unicode::utf8::decode_error_code::unsupported_leading, "utf8 decode result copied");
        check(err.last_code_unit == 0xFF && err.last_code_unit_offset == 0, "utf8 last code unit and offset");
    }

    void test_parser_payloads()
    {
        const ret::unary_operation neg{ .operand = pdn::type::i32{ 5 }, .operand_type = pdn::type_code::i32, .negative = true };
        check(neg.operand_type == pdn::type_code::i32 && neg.negative, "unary_operation fields");

        const ret::casting_msg cast{ .operand = pdn::type::boolean{ true }, .source_type = pdn::type_code::boolean, .target_type = pdn::type_code::i32 };
        check(cast.source_type == pdn::type_code::boolean && cast.target_type == pdn::type_code::i32, "casting_msg fields");

        const ret::error_token et{ .value = pdn::token<pdn::error_msg_char>{ .code = pdn::pdn_token_code::semicolon } };
        check(et.value.code == pdn::pdn_token_code::semicolon, "error_token keeps the token");
    }
}

int main()
{
    test_payload_values();
    test_codec_payload();
    test_parser_payloads();

    if (fails != 0)
    {
        std::printf("%d check(s) failed\n", fails);
        return 1;
    }
    std::printf("probe ok\n");
    return 0;
}
