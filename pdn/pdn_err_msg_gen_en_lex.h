#ifndef PDN_Header_pdn_err_msg_gen_en_lex
#define PDN_Header_pdn_err_msg_gen_en_lex

#include <variant>

#include "pdn_exception.h"
#include "pdn_error_string.h"
#include "pdn_lexical_error_code.h"
#include "pdn_source_position.h"
#include "pdn_raw_error_message_type.h"
#include "pdn_raw_error_message_variant.h"

#include "pdn_err_msg_gen_utility.h"

namespace pdn::dev_util
{
	inline auto err_msg_gen_en(lexical_error_code errc, source_position, raw_err_v_cref raw) -> error_msg_string
	{
		using namespace err_msg_gen_util::lexical_err_msg_gen_util;
		using namespace error_message_literals;
		using enum lexical_error_code;
		auto src = u8"todo"_em;
		switch (errc)
		{
		case not_unicode_scalar_value:
			return u8"pdn lexer requires unicode scalar value: "_em + get_code_point_hex(raw);
		case unacceptable_character:
			return u8"unacceptable character: \""_em + src + u8"\""_em;
		case at_value_not_found:
			return u8"at value not found: \""_em + src + u8"\""_em;
		case identifier_string_terminated_by_LF:
			return u8"identifier string terminated by LF: \""_em + src + u8"\""_em;
		case string_literal_terminated_by_LF:
			return u8"string literal terminated by LF: \""_em + src + u8"\""_em;
		case character_literal_terminated_by_LF:
			return u8"character literal terminated by LF: \""_em + src + u8"\""_em;
		case character_literal_length_is_zero:
			return u8"character literal length cannot be equal to 0: \""_em + src + u8"\""_em;
		case character_literal_length_is_greater_than_one:
			return u8"character literal length cannot be greater than 1: \""_em + src + u8"\""_em;

		case unterminated_block_comment:
			return u8"unterminated /* comment"_em;
		case unterminated_nested_block_comment:
			return u8"unterminated </ comment"_em;
		case identifier_string_missing_terminating_character:
			return u8"identifier string missing terminating ` character: \""_em + src + u8"\""_em;
		case string_missing_terminating_character:
			return u8"string missing terminating \" character: \""_em + src + u8"\""_em;
		case character_missing_terminating_character:
			return u8"character missing terminating ' character: \""_em + src + u8"\""_em;
		case identifier_raw_string_missing_terminating_sequence:
			return u8"identifier raw string missing terminating )delimiters` sequence: \""_em + src + u8"\""_em;
		case raw_string_missing_terminating_sequence:
			return u8"raw string missing terminating )delimiters\" sequence: \""_em + src + u8"\""_em;

		case invalid_octal_number:
			return u8"invalid octal number: \""_em + src + u8"\", reinterpreting it as decimal integer"_em;
		case fp_dec_expect_exponent:
			return u8"expect exponent part: \""_em + src + u8"\""_em;
		case fp_hex_expect_exponent:
			return u8"hexadecimal floating-point literal requires an exponent: \""_em + src + u8"\""_em;
		case bin_prefix_expect_bin_number_sequence:
			return u8"expect binary number sequence: \""_em + src + u8"\""_em;
		case hex_prefix_expect_hex_number_sequence:
			return u8"expect hexadecimal number sequence: \""_em + src + u8"\""_em;
		case fp_hex_expect_decimal_part:
			return u8"expect hexadecimal decimal part: \""_em + src + u8"\""_em;
		case more_than_one_separators_may_between_numbers:
			return u8"more than one separators may between numbers: \""_em + src + u8"\""_em;
		case number_cannot_end_with_separator:
			return u8"number cannot end with separator: \""_em + src + u8"\""_em;

			// ESCAPE ERROR
			// escape \o{...}
		case escape_error_o_not_followed_by_left_brackets:
			return u8"escape error: \""_em + src + u8"\" not followed by '{'"_em;
		case escape_error_o_not_terminated_with_right_brackets:
			return u8"escape error: \""_em + src + u8"\" not followed by '}'"_em;
		case escape_error_o_empty_delimited_escape_sequence:
			return u8"escape error: \""_em + src + u8"\" empty delimited escape sequence"_em;

			// escape \x... \x{...}
		case escape_error_x_used_with_no_following_hex_digits:
			return u8"escape error: \""_em + src + u8"\" not followed by hexadecimal digit or '{'"_em;
		case escape_error_x_not_terminated_with_right_brackets:
			return u8"escape error: \""_em + src + u8"\" not followed by '}'"_em;
		case escape_error_x_empty_delimited_escape_sequence:
			return u8"escape error: \""_em + src + u8"\" empty delimited escape sequence"_em;

			// escape \Unnnnnnnn \unnnn \u{...}
		case escape_error_U_incomplete_universal_character_name:
		case escape_error_u_incomplete_universal_character_name:
			return u8"escape error: \""_em + src + u8"\" incomplete universal character name"_em;
		case escape_error_u_not_terminated_with_right_brackets:
			return u8"escape error: \""_em + src + u8"\" not followed by '}'"_em;
		case escape_error_u_empty_delimited_escape_sequence:
			return u8"escape error: \""_em + src + u8"\" empty delimited escape sequence"_em;

			// escape from_chars
		case escape_error_from_chars_parsing_incomplete:
			return u8"escape error: from_chars parsing incomplete \""_em + src + u8"\""_em;
		case escape_error_from_chars_errc_result_out_of_range:
			return u8"escape error: from_chars error result_out_of_range \""_em + src + u8"\""_em;
		case escape_error_from_chars_errc_invalid_argument:
			return u8"escape error: from_chars error invalid_argument \""_em + src + u8"\""_em;
		case escape_error_from_chars_errc_other:
			return u8"escape error: from_chars error (not result_out_of_range and invalid_argument) \""_em + src + u8"\""_em;

			// escape other
		case escape_error_not_unicode_scalar_value:
			return u8"escape error: \""_em + src + u8"\" not unicode scalar value"_em;
		case escape_error_unknown_escape_sequence:
			return u8"escape error: unknown escape sequence \""_em + src + u8"\""_em;

			// RAW STRING / IDENTIFIER RAW STRING D SEQUENCE ERROR
		case d_seq_error_raw_string_delimiter_longer_than_16_characters:
			return u8"raw string delimiter sequence longer than 16 characters: \""_em + src + u8"\""_em;
		case d_seq_error_invalid_character_in_raw_string_delimiter:
			return u8"invalid character in raw string delimiter: \""_em + src + u8"\""_em;
		case d_seq_error_cannot_find_end_sign:
			return u8"raw string delimiter sequence not close: \""_em + src + u8"\""_em;

			// NUMBER FROM CHARS ERROR
		case number_from_chars_parsing_incomplete:
			return u8"number from_chars parsing incomplete \""_em + src + u8"\""_em;
		case number_from_chars_errc_result_out_of_range:
			return u8"number from_chars error result_out_of_range \""_em + src + u8"\""_em;
		case number_from_chars_errc_invalid_argument:
			return u8"number from_chars error invalid_argument \""_em + src + u8"\""_em;
		case number_from_chars_errc_other:
			return u8"number from_chars error (not result_out_of_range and invalid_argument) \""_em + src + u8"\""_em;
		case inner_error_binary_integer_without_0b_prefix:
			return u8"inner lexer error binary integer sequence without 0b|0B prefix: \""_em + src + u8"\""_em;
		case inner_error_hexadecimal_integer_without_0x_prefix:
			return u8"inner lexer error hexadecimal integer sequence without 0x|0X prefix: \""_em + src + u8"\""_em;
		case inner_error_hexadecimal_floating_point_without_0x_prefix:
			return u8"inner lexer error hexadecimal floating point sequence without 0x|0X prefix: \""_em + src + u8"\""_em;
		case inner_error_make_non_final_dfa_state_to_token:
			return u8"inner lexer error non-final dfa state to token"_em;

		default:
			break;
		}
		return u8"lexical error, error_message_generator_en unresolved: \""_em + src + u8"\""_em;
	}
}

#endif
