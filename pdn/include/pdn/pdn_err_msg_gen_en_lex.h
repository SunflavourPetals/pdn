#ifndef PDN_Header_pdn_err_msg_gen_en_lex
#define PDN_Header_pdn_err_msg_gen_en_lex

#include <cassert>
#include <variant>

#include "pdn_error_string.h"
#include "pdn_lexical_error_code.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::detail
{
	// English text for the lexical errors, one case per enumerator. There is deliberately no default:
	// a new code without a message must fail -Wswitch instead of falling back to a generic text.
	// The position is not used here; the caller prefixes the message with "code(line:column)".
	// Payload readers are named after the code, see err_msg_gen_util::lexical_err_msg_gen_util.
	inline auto err_msg_gen_en(lexical_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util::lexical_err_msg_gen_util;
		using namespace err_msg_gen_util;
		using namespace error_message_literals;
		using enum lexical_error_code;
		switch (errc)
		{
		case not_unicode_scalar_value:
			return u8"PDN lexer requires a Unicode scalar value: "_em + get_code_point_hex(raw);
		case unacceptable_character:
			return u8"unacceptable character: "_em + get_quoted_slashes_s(raw);

		case identifier_string_terminated_by_LF:
			return u8"identifier string terminated by LF: "_em + get_quoted_slashes_s(raw);
		case string_literal_terminated_by_LF:
			return u8"string literal terminated by LF: "_em + get_quoted_slashes_s(raw);
		case character_literal_terminated_by_LF:
			return u8"character literal terminated by LF: "_em + get_single_quoted_slashes_s(raw);
		case character_literal_length_is_zero:
			return u8"character literal "_em + get_single_quoted_slashes_s_for_cle(raw)
				+ u8" is empty, expected exactly one code point"_em;
		case character_literal_length_is_greater_than_one:
			return u8"character literal "_em + get_single_quoted_slashes_s_for_cle(raw)
				+ u8" holds "_em + get_cle_cp_length(raw) + u8" code points, expected exactly one"_em;

		case unterminated_block_comment:
			return u8"unterminated block comment: /*"_em;
		case unterminated_nested_block_comment:
			return u8"unterminated nested block comment: </"_em;

		case identifier_string_missing_terminating_character:
			return u8"identifier string is missing its terminating backtick, content: "_em + get_quoted_slashes_s(raw);
		case string_missing_terminating_character:
			return u8"string literal is missing its terminating double quote, content: "_em + get_quoted_slashes_s(raw);
		case character_missing_terminating_character:
			return u8"character literal is missing its terminating single quote, content: "_em + get_single_quoted_slashes_s(raw);
		case identifier_raw_string_missing_terminating_sequence:
			return u8"identifier raw string is missing its terminating sequence )"_em
				+ get_slashes_d_seq_for_mts(raw) + u8"`, content: "_em + get_quoted_slashes_s_for_mts(raw);
		case raw_string_missing_terminating_sequence:
			return u8"raw string is missing its terminating sequence )"_em
				+ get_slashes_d_seq_for_mts(raw) + u8"\", content: "_em + get_quoted_slashes_s_for_mts(raw);

		case invalid_octal_number:
			return u8"invalid octal number: "_em + get_err_s(raw) + u8", interpreting it as a decimal integer"_em;
		case fp_dec_expect_exponent:
			return u8"expected an exponent: "_em + get_err_s(raw);
		case fp_hex_expect_exponent:
			return u8"hexadecimal floating-point literal requires an exponent: "_em + get_err_s(raw);
		case bin_prefix_expect_bin_number_sequence:
			return u8"expected a binary digit sequence: "_em + get_err_s(raw);
		case hex_prefix_expect_hex_number_sequence:
			return u8"expected a hexadecimal digit sequence: "_em + get_err_s(raw);
		case fp_hex_expect_decimal_part:
			return u8"expected a hexadecimal fractional part: "_em + get_err_s(raw);
		case more_than_one_separators_between_numbers:
			return u8"more than one separator between digits: "_em + get_err_s(raw);
		case number_cannot_end_with_separator:
			return u8"number cannot end with a separator: "_em + get_s_for_ews(raw);

			// NUMBER FROM CHARS ERROR
		case number_from_chars_parsing_incomplete:
			return u8"number from_chars parsing incomplete, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", incomplete part: "_em + get_quoted_from_chars_incomplete_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);
		case number_from_chars_errc_result_out_of_range:
			return u8"number from_chars error result_out_of_range, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", code: "_em + get_from_chars_errc_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);
		case number_from_chars_errc_invalid_argument:
			return u8"number from_chars error invalid_argument, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", code: "_em + get_from_chars_errc_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);
		case number_from_chars_errc_other:
			return u8"number from_chars error other, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", code: "_em + get_from_chars_errc_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);

			// ESCAPE ERROR
		case escape_error_unknown_escape_sequence:
			return u8"unknown escape sequence: "_em + get_esc_quoted_s(raw);

			// escape \o{...}
		case escape_error_o_not_followed_by_left_brackets:
			return get_esc_quoted_s(raw) + u8" must be followed by '{'"_em;
		case escape_error_o_not_terminated_with_right_brackets:
			return get_esc_quoted_s(raw) + u8" is not terminated by '}'"_em;
		case escape_error_o_empty_delimited_escape_sequence:
			return get_esc_quoted_s(raw) + u8" contains an empty delimited escape sequence"_em;

			// escape \x... \x{...}
		case escape_error_x_not_terminated_with_right_brackets:
			return get_esc_quoted_s(raw) + u8" is not terminated by '}'"_em;
		case escape_error_x_empty_delimited_escape_sequence:
			return get_esc_quoted_s(raw) + u8" contains an empty delimited escape sequence"_em;
		case escape_error_x_used_with_no_following_hex_digits:
			return get_esc_quoted_s(raw) + u8" must be followed by a hexadecimal digit or '{'"_em;

			// escape \Unnnnnnnn \unnnn \u{...}
		case escape_error_U_incomplete_universal_character_name:
		case escape_error_u_incomplete_universal_character_name:
			return get_esc_quoted_s(raw) + u8" is an incomplete universal character name"_em;
		case escape_error_u_not_terminated_with_right_brackets:
			return get_esc_quoted_s(raw) + u8" is not terminated by '}'"_em;
		case escape_error_u_empty_delimited_escape_sequence:
			return get_esc_quoted_s(raw) + u8" contains an empty delimited escape sequence"_em;

			// escape result is not unicode scalar value
		case escape_error_not_unicode_scalar_value:
			return u8"escape result "_em + get_quoted_seq_from_esc_not_u_scalar(raw)
				+ u8" (= "_em + get_value_from_esc_not_u_scalar(raw)
				+ u8") is not a Unicode scalar value"_em;

			// escape from_chars
		case escape_error_from_chars_parsing_incomplete:
			return u8"escape from_chars parsing incomplete, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", incomplete part: "_em + get_quoted_from_chars_incomplete_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);
		case escape_error_from_chars_errc_result_out_of_range:
			return u8"escape from_chars error result_out_of_range, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", code: "_em + get_from_chars_errc_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);
		case escape_error_from_chars_errc_invalid_argument:
			return u8"escape from_chars error invalid_argument, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", code: "_em + get_from_chars_errc_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);
		case escape_error_from_chars_errc_other:
			return u8"escape from_chars error other, source: "_em + get_quoted_from_chars_src_s(raw)
				+ u8", code: "_em + get_from_chars_errc_s(raw)
				+ u8", type: "_em + get_from_chars_desc_s(raw);

			// RAW STRING / IDENTIFIER RAW STRING D SEQUENCE ERROR
		case d_seq_error_raw_string_delimiter_longer_than_16_characters:
			return u8"raw string delimiter is longer than 16 characters: "_em + add_quote(get_prefix_and_d_seq_with_par(raw));
		case d_seq_error_invalid_character_in_raw_string_delimiter:
			return u8"invalid character in raw string delimiter: "_em + add_quote(get_prefix_and_d_seq_with_par(raw));
		case d_seq_error_cannot_find_end_sign:
			return u8"raw string is not closed: "_em + add_quote(get_prefix_and_d_seq(raw));
		}
		assert(false && "[pdn] lexical error code without an English message");
		return u8"unhandled lexical error code"_em;
	}
}

#endif
