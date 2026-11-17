#include <pdn/pdn_err_msg_gen_en_lex.h>

#include <cstdio>
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

    namespace rt = pdn::raw_error_message_type;
    using pdn::error_msg_string;
    using pdn::lexical_error_code;
    using variant_t = pdn::raw_error_message_variant;
    using pos = pdn::source_position;

    static_assert(::std::is_same_v<decltype(pdn::detail::err_msg_gen_en(
        lexical_error_code::unacceptable_character, pos{}, variant_t{ rt::error_string{ u8"x" } })), error_msg_string>);

    auto gen(lexical_error_code code, variant_t raw) -> error_msg_string
    {
        return pdn::detail::err_msg_gen_en(code, pos{}, raw);
    }

    auto err_s(const char8_t* text) -> variant_t
    {
        return variant_t{ rt::error_string{ text } };
    }

    auto from_chars(const char8_t* sequence, ::std::ptrdiff_t offset, ::std::errc ec, rt::number_type type) -> variant_t
    {
        return variant_t{ rt::from_chars_error{ sequence, offset, ec, type } };
    }

    void test_text_and_comments()
    {
        check(gen(lexical_error_code::not_unicode_scalar_value, variant_t{ rt::not_unicode_scalar_value{ 0xD800u } })
            == u8"PDN lexer requires a Unicode scalar value: 0x0000d800", "not a Unicode scalar value");
        check(gen(lexical_error_code::unacceptable_character, err_s(u8"\xe4\xb8\xad"))
            == u8"unacceptable character: \"\xe4\xb8\xad\"", "unacceptable character");
        check(gen(lexical_error_code::identifier_string_terminated_by_LF, err_s(u8"abc"))
            == u8"identifier string terminated by LF: \"abc\"", "identifier string terminated by LF");
        check(gen(lexical_error_code::string_literal_terminated_by_LF, err_s(u8"abc"))
            == u8"string literal terminated by LF: \"abc\"", "string literal terminated by LF");
        check(gen(lexical_error_code::character_literal_terminated_by_LF, err_s(u8"a"))
            == u8"character literal terminated by LF: 'a'", "character literal terminated by LF");
        check(gen(lexical_error_code::character_literal_length_is_zero, variant_t{ rt::character_length_error{ u8"", 0 } })
            == u8"character literal '' is empty, expected exactly one code point", "empty character literal");
        check(gen(lexical_error_code::character_literal_length_is_greater_than_one, variant_t{ rt::character_length_error{ u8"abc", 3 } })
            == u8"character literal 'abc' holds 3 code points, expected exactly one", "character literal with three code points");
        check(gen(lexical_error_code::unterminated_block_comment, variant_t{ ::std::monostate{} })
            == u8"unterminated block comment: /*", "unterminated block comment");
        check(gen(lexical_error_code::unterminated_nested_block_comment, variant_t{ ::std::monostate{} })
            == u8"unterminated nested block comment: </", "unterminated nested block comment");
    }

    void test_missing_terminators()
    {
        check(gen(lexical_error_code::identifier_string_missing_terminating_character, err_s(u8"abc"))
            == u8"identifier string is missing its terminating backtick, content: \"abc\"", "identifier string missing backtick");
        check(gen(lexical_error_code::string_missing_terminating_character, err_s(u8"abc"))
            == u8"string literal is missing its terminating double quote, content: \"abc\"", "string missing double quote");
        check(gen(lexical_error_code::character_missing_terminating_character, err_s(u8"a"))
            == u8"character literal is missing its terminating single quote, content: 'a'", "character missing single quote");
        check(gen(lexical_error_code::identifier_raw_string_missing_terminating_sequence,
                variant_t{ rt::missing_terminating_sequence{ u8"body", u8"##", true } })
            == u8"identifier raw string is missing its terminating sequence )##`, content: \"@`##(body\"", "identifier raw string missing terminator");
        check(gen(lexical_error_code::raw_string_missing_terminating_sequence,
                variant_t{ rt::missing_terminating_sequence{ u8"body", u8"##", false } })
            == u8"raw string is missing its terminating sequence )##\", content: \"@\"##(body\"", "raw string missing terminator");
    }

    void test_numbers()
    {
        check(gen(lexical_error_code::invalid_octal_number, err_s(u8"0o9"))
            == u8"invalid octal number: 0o9, interpreting it as a decimal integer", "invalid octal number");
        check(gen(lexical_error_code::fp_dec_expect_exponent, err_s(u8"1.5e"))
            == u8"expected an exponent: 1.5e", "decimal float exponent");
        check(gen(lexical_error_code::fp_hex_expect_exponent, err_s(u8"0x1p"))
            == u8"hexadecimal floating-point literal requires an exponent: 0x1p", "hex float exponent");
        check(gen(lexical_error_code::bin_prefix_expect_bin_number_sequence, err_s(u8"0b"))
            == u8"expected a binary digit sequence: 0b", "binary digit sequence");
        check(gen(lexical_error_code::hex_prefix_expect_hex_number_sequence, err_s(u8"0x"))
            == u8"expected a hexadecimal digit sequence: 0x", "hexadecimal digit sequence");
        check(gen(lexical_error_code::fp_hex_expect_decimal_part, err_s(u8"0x1."))
            == u8"expected a hexadecimal fractional part: 0x1.", "hexadecimal fractional part");
        check(gen(lexical_error_code::more_than_one_separators_between_numbers, err_s(u8"1__2"))
            == u8"more than one separator between digits: 1__2", "more than one separator");
        check(gen(lexical_error_code::number_cannot_end_with_separator,
                variant_t{ rt::number_end_with_separator{ u8"1_", rt::number_type::dec_integer } })
            == u8"number cannot end with a separator: 1_", "number ending with a separator");
    }

    void test_from_chars()
    {
        check(gen(lexical_error_code::number_from_chars_parsing_incomplete,
                from_chars(u8"12x", 2, ::std::errc::invalid_argument, rt::number_type::dec_integer))
            == u8"number from_chars parsing incomplete, source: \"12x\", incomplete part: \"12\", type: integer, base 10",
            "number from_chars incomplete");
        check(gen(lexical_error_code::number_from_chars_errc_result_out_of_range,
                from_chars(u8"12x", 2, ::std::errc::result_out_of_range, rt::number_type::dec_integer))
            == u8"number from_chars error result_out_of_range, source: \"12x\", code: 34, type: integer, base 10",
            "number from_chars out of range");
        check(gen(lexical_error_code::number_from_chars_errc_invalid_argument,
                from_chars(u8"12x", 2, ::std::errc::invalid_argument, rt::number_type::dec_integer))
            == u8"number from_chars error invalid_argument, source: \"12x\", code: 22, type: integer, base 10",
            "number from_chars invalid argument");
        check(gen(lexical_error_code::number_from_chars_errc_other,
                from_chars(u8"12x", 2, static_cast<::std::errc>(0), rt::number_type::dec_integer))
            == u8"number from_chars error other, source: \"12x\", code: 0, type: integer, base 10",
            "number from_chars other");
    }

    void test_escapes()
    {
        check(gen(lexical_error_code::escape_error_unknown_escape_sequence, err_s(u8"q"))
            == u8"unknown escape sequence: \"\\q\"", "unknown escape sequence");
        check(gen(lexical_error_code::escape_error_o_not_followed_by_left_brackets, err_s(u8"o"))
            == u8"\"\\o\" must be followed by '{'", "octal escape without a bracket");
        check(gen(lexical_error_code::escape_error_o_not_terminated_with_right_brackets, err_s(u8"o{12"))
            == u8"\"\\o{12\" is not terminated by '}'", "octal escape not terminated");
        check(gen(lexical_error_code::escape_error_o_empty_delimited_escape_sequence, err_s(u8"o{}"))
            == u8"\"\\o{}\" contains an empty delimited escape sequence", "empty octal escape");
        check(gen(lexical_error_code::escape_error_x_not_terminated_with_right_brackets, err_s(u8"x{12"))
            == u8"\"\\x{12\" is not terminated by '}'", "hex escape not terminated");
        check(gen(lexical_error_code::escape_error_x_empty_delimited_escape_sequence, err_s(u8"x{}"))
            == u8"\"\\x{}\" contains an empty delimited escape sequence", "empty hex escape");
        check(gen(lexical_error_code::escape_error_x_used_with_no_following_hex_digits, err_s(u8"x"))
            == u8"\"\\x\" must be followed by a hexadecimal digit or '{'", "hex escape without digits");
        check(gen(lexical_error_code::escape_error_U_incomplete_universal_character_name, err_s(u8"U0001"))
            == u8"\"\\U0001\" is an incomplete universal character name", "incomplete \\U name");
        check(gen(lexical_error_code::escape_error_u_incomplete_universal_character_name, err_s(u8"u12"))
            == u8"\"\\u12\" is an incomplete universal character name", "incomplete \\u name");
        check(gen(lexical_error_code::escape_error_u_not_terminated_with_right_brackets, err_s(u8"u{12"))
            == u8"\"\\u{12\" is not terminated by '}'", "\\u escape not terminated");
        check(gen(lexical_error_code::escape_error_u_empty_delimited_escape_sequence, err_s(u8"u{}"))
            == u8"\"\\u{}\" contains an empty delimited escape sequence", "empty \\u escape");
        check(gen(lexical_error_code::escape_error_not_unicode_scalar_value,
                variant_t{ rt::escape_not_unicode_scalar_value{ u8"u{110000}", 0x110000u } })
            == u8"escape result \"\\u{110000}\" (= 0x110000) is not a Unicode scalar value", "escape result is not a scalar value");
        check(gen(lexical_error_code::escape_error_from_chars_parsing_incomplete,
                from_chars(u8"u12g", 3, ::std::errc::invalid_argument, rt::number_type::hex_integer))
            == u8"escape from_chars parsing incomplete, source: \"u12g\", incomplete part: \"u12\", type: integer, base 16",
            "escape from_chars incomplete");
        check(gen(lexical_error_code::escape_error_from_chars_errc_result_out_of_range,
                from_chars(u8"u12g", 3, ::std::errc::result_out_of_range, rt::number_type::hex_integer))
            == u8"escape from_chars error result_out_of_range, source: \"u12g\", code: 34, type: integer, base 16",
            "escape from_chars out of range");
        check(gen(lexical_error_code::escape_error_from_chars_errc_invalid_argument,
                from_chars(u8"u12g", 3, ::std::errc::invalid_argument, rt::number_type::hex_integer))
            == u8"escape from_chars error invalid_argument, source: \"u12g\", code: 22, type: integer, base 16",
            "escape from_chars invalid argument");
        check(gen(lexical_error_code::escape_error_from_chars_errc_other,
                from_chars(u8"u12g", 3, static_cast<::std::errc>(0), rt::number_type::hex_integer))
            == u8"escape from_chars error other, source: \"u12g\", code: 0, type: integer, base 16",
            "escape from_chars other");
    }

    void test_delimiter()
    {
        check(gen(lexical_error_code::d_seq_error_raw_string_delimiter_longer_than_16_characters,
                variant_t{ rt::delimiter_error{ u8"##", false } })
            == u8"raw string delimiter is longer than 16 characters: \"@\\\"##(\"", "delimiter longer than 16 characters");
        check(gen(lexical_error_code::d_seq_error_invalid_character_in_raw_string_delimiter,
                variant_t{ rt::delimiter_error{ u8"##", true } })
            == u8"invalid character in raw string delimiter: \"@`##(\"", "invalid character in delimiter");
        check(gen(lexical_error_code::d_seq_error_cannot_find_end_sign,
                variant_t{ rt::delimiter_error{ u8"##", false } })
            == u8"raw string is not closed: \"@\\\"##\"", "raw string not closed");
    }
}

int main()
{
    test_text_and_comments();
    test_missing_terminators();
    test_numbers();
    test_from_chars();
    test_escapes();
    test_delimiter();
    if (fails == 0)
    {
        std::printf("all pdn_err_msg_gen_en_lex checks passed\n");
    }
    return fails == 0 ? 0 : 1;
}
