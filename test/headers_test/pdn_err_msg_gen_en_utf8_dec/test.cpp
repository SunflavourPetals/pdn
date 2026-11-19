#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <variant>

#include <pdn/pdn_err_msg_gen_en_utf8_dec.h>

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

    namespace rt = pdn::raw_error_message_type;
    namespace utf8 = pdn::unicode::utf8;
    using pdn::error_msg_string;
    using variant_t = pdn::raw_error_message_variant;
    using pos = pdn::source_position;

    static_assert(::std::is_same_v<decltype(pdn::detail::err_msg_gen_en(
        utf8::decode_error_code::invalid_code_point, pos{},
        variant_t{ rt::utf8_decode_error{ utf8::decode_result{}, utf8::code_unit_t{}, 0 } })), error_msg_string>);

    // The decode result is default constructed: only the code unit and the offset vary here, the
    // decoder fills the value and the distance in real runs. The offset is the byte offset of the
    // sequence, so the same input offset is scaled per code unit width.
    auto msg(utf8::decode_error_code code, utf8::code_unit_t unit, ::std::size_t offset) -> error_msg_string
    {
        return pdn::detail::err_msg_gen_en(code, pos{},
            variant_t{ rt::utf8_decode_error{ utf8::decode_result{}, unit, offset } });
    }

    const auto tail = u8", in the sequence at offset 0x0004 (add 3 if the input starts with a BOM), 1 code unit was read";

    void test_unit_messages()
    {
        check(msg(utf8::decode_error_code::invalid_code_point, utf8::code_unit_t{ 0x80 }, 4)
            == (error_msg_string{ u8"invalid code point: 0x0000" } + tail), "invalid code point");
        check(msg(utf8::decode_error_code::non_code_point, utf8::code_unit_t{ 0x80 }, 4)
            == (error_msg_string{ u8"not a code point: 0x0000" } + tail), "not a code point");
        check(msg(utf8::decode_error_code::incomplete_sequence, utf8::code_unit_t{ 0x80 }, 4)
            == (error_msg_string{ u8"incomplete sequence (expected a UTF-8 trailing code unit), code unit: 0x80" } + tail),
            "incomplete sequence");
        check(msg(utf8::decode_error_code::trailing_as_start, utf8::code_unit_t{ 0x80 }, 4)
            == (error_msg_string{ u8"trailing code unit where a UTF-8 leading code unit is expected, code unit: 0x80" } + tail),
            "trailing as start");
        check(msg(utf8::decode_error_code::unsupported_leading, utf8::code_unit_t{ 0xfe }, 4)
            == (error_msg_string{ u8"unsupported UTF-8 leading code unit: 0xfe" } + tail), "unsupported leading");
        check(msg(utf8::decode_error_code::non_shortest_sequence, utf8::code_unit_t{ 0x80 }, 4)
            == (error_msg_string{ u8"non-shortest UTF-8 encoding" } + tail), "non-shortest sequence");
    }

    void test_eof_messages()
    {
        check(msg(utf8::decode_error_code::unexpected_eof_offset0, utf8::code_unit_t{ 0x80 }, 4)
            == u8"unexpected end of input while reading code unit 1 of the sequence at offset 0x0004 (add 3 if the input starts with a BOM)",
            "eof at code unit 1");
        check(msg(utf8::decode_error_code::unexpected_eof_offset1, utf8::code_unit_t{ 0x80 }, 4)
            == u8"unexpected end of input while reading code unit 2 of the sequence at offset 0x0004 (add 3 if the input starts with a BOM)",
            "eof at code unit 2");
        check(msg(utf8::decode_error_code::unexpected_eof_offset2, utf8::code_unit_t{ 0x80 }, 4)
            == u8"unexpected end of input while reading code unit 3 of the sequence at offset 0x0004 (add 3 if the input starts with a BOM)",
            "eof at code unit 3");
        check(msg(utf8::decode_error_code::unexpected_eof_offset3, utf8::code_unit_t{ 0x80 }, 4)
            == u8"unexpected end of input while reading code unit 4 of the sequence at offset 0x0004 (add 3 if the input starts with a BOM)",
            "eof at code unit 4");
        check(msg(utf8::decode_error_code::unexpected_eof_offset4, utf8::code_unit_t{ 0x80 }, 4)
            == u8"unexpected end of input while reading code unit 5 of the sequence at offset 0x0004 (add 3 if the input starts with a BOM)",
            "eof at code unit 5");
        check(msg(utf8::decode_error_code::unexpected_eof_offset5, utf8::code_unit_t{ 0x80 }, 4)
            == u8"unexpected end of input while reading code unit 6 of the sequence at offset 0x0004 (add 3 if the input starts with a BOM)",
            "eof at code unit 6");
    }
}

int main()
{
    test_unit_messages();
    test_eof_messages();
    if (fails == 0)
    {
        std::printf("all pdn_err_msg_gen_en_utf8_dec checks passed\n");
    }
    return fails == 0 ? 0 : 1;
}
