#include <cstddef>
#include <cstdio>
#include <type_traits>
#include <variant>

#include <pdn/pdn_err_msg_gen_en_utf32_dec.h>

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
    namespace utf32 = pdn::unicode::utf32;
    using pdn::error_msg_string;
    using variant_t = pdn::raw_error_message_variant;
    using pos = pdn::source_position;

    static_assert(::std::is_same_v<decltype(pdn::detail::err_msg_gen_en(
        utf32::decode_error_code::invalid_code_point, pos{},
        variant_t{ rt::utf32_decode_error{ utf32::decode_result{}, utf32::code_unit_t{}, 0 } })), error_msg_string>);

    // The decode result is default constructed: only the code unit and the offset vary here, the
    // decoder fills the value and the distance in real runs. The offset is a byte offset, so a code
    // unit offset of eight is reported as 0x0020.
    auto msg(utf32::decode_error_code code, utf32::code_unit_t unit, ::std::size_t offset) -> error_msg_string
    {
        return pdn::detail::err_msg_gen_en(code, pos{},
            variant_t{ rt::utf32_decode_error{ utf32::decode_result{}, unit, offset } });
    }

    const auto tail = u8", in the sequence at offset 0x0020 (add 4 if the input starts with a BOM), 1 code unit was read";

    void test_messages()
    {
        check(msg(utf32::decode_error_code::invalid_code_point, utf32::code_unit_t{ 0x11000 }, 8)
            == (error_msg_string{ u8"invalid code point: 0x00000000" } + tail), "invalid code point");
        check(msg(utf32::decode_error_code::non_code_point, utf32::code_unit_t{ 0x11000 }, 8)
            == (error_msg_string{ u8"not a code point: 0x00000000" } + tail), "not a code point");
        check(msg(utf32::decode_error_code::unexpected_eof, utf32::code_unit_t{ 0x11000 }, 8)
            == u8"unexpected end of input while reading a code unit of the sequence at offset 0x0020 (add 4 if the input starts with a BOM)",
            "unexpected eof");
    }
}

int main()
{
    test_messages();
    if (fails == 0)
    {
        std::printf("all pdn_err_msg_gen_en_utf32_dec checks passed\n");
    }
    return fails == 0 ? 0 : 1;
}
