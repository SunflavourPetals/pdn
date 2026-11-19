#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <variant>

#include <pdn/pdn_err_msg_gen_en_utf16_dec.h>

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
    namespace utf16 = pdn::unicode::utf16;
    using pdn::error_msg_string;
    using variant_t = pdn::raw_error_message_variant;
    using pos = pdn::source_position;

    static_assert(::std::is_same_v<decltype(pdn::detail::err_msg_gen_en(
        utf16::decode_error_code::invalid_code_point, pos{},
        variant_t{ rt::utf16_decode_error{ utf16::decode_result{}, utf16::code_unit_t{}, 0 } })), error_msg_string>);

    // The decode result is default constructed: only the code unit and the offset vary here, the
    // decoder fills the value and the distance in real runs. The offset is a byte offset, so a code
    // unit offset of four is reported as 0x0008.
    auto msg(utf16::decode_error_code code, utf16::code_unit_t unit, ::std::size_t offset) -> error_msg_string
    {
        return pdn::detail::err_msg_gen_en(code, pos{},
            variant_t{ rt::utf16_decode_error{ utf16::decode_result{}, unit, offset } });
    }

    const auto tail = u8", in the sequence at offset 0x0008 (add 2 if the input starts with a BOM), 1 code unit was read";

    void test_unit_messages()
    {
        check(msg(utf16::decode_error_code::invalid_code_point, utf16::code_unit_t{ 0xdc00 }, 4)
            == (error_msg_string{ u8"invalid code point: 0x0000" } + tail), "invalid code point");
        check(msg(utf16::decode_error_code::non_code_point, utf16::code_unit_t{ 0xdc00 }, 4)
            == (error_msg_string{ u8"not a code point: 0x0000" } + tail), "not a code point");
        check(msg(utf16::decode_error_code::incomplete_sequence, utf16::code_unit_t{ 0xdc00 }, 4)
            == (error_msg_string{ u8"incomplete sequence (expected a UTF-16 trailing surrogate), code unit: 0xdc00" } + tail),
            "incomplete sequence");
        check(msg(utf16::decode_error_code::trailing_as_start, utf16::code_unit_t{ 0xdc00 }, 4)
            == (error_msg_string{ u8"lone UTF-16 trailing surrogate where a leading surrogate is expected, code unit: 0xdc00" } + tail),
            "trailing as start");
    }

    void test_eof_messages()
    {
        check(msg(utf16::decode_error_code::unexpected_eof_offset0, utf16::code_unit_t{ 0xdc00 }, 4)
            == u8"unexpected end of input while reading the first code unit of the sequence at offset 0x0008 (add 2 if the input starts with a BOM)",
            "eof before the first code unit");
        check(msg(utf16::decode_error_code::unexpected_eof_offset1, utf16::code_unit_t{ 0xdc00 }, 4)
            == u8"unexpected end of input while reading the UTF-16 trailing surrogate of the sequence at offset 0x0008 (add 2 if the input starts with a BOM)",
            "eof at the trailing surrogate");
    }
}

int main()
{
    test_unit_messages();
    test_eof_messages();
    if (fails == 0)
    {
        std::printf("all pdn_err_msg_gen_en_utf16_dec checks passed\n");
    }
    return fails == 0 ? 0 : 1;
}
